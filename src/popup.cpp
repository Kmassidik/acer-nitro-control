#include "popup.h"
#include "config.h"
#include "control.h"
#include "levelbutton.h"
#include "sensors.h"
#include "theme.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

static QFrame *makeHLine(QWidget *parent)
{
    auto *ln = new QFrame(parent);
    ln->setFrameShape(QFrame::HLine);
    ln->setFixedHeight(1);
    ln->setStyleSheet("background: rgba(94,110,135,0.35); border: none;");
    return ln;
}

static QWidget *tempRow(const QString &name, QLabel *fan, QLabel *temp,
                        QProgressBar *bar, QWidget *parent)
{
    auto *w = new QWidget(parent);
    auto *wl = new QVBoxLayout(w);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(4);

    auto *head = new QHBoxLayout;
    head->setSpacing(8);
    auto *lbl = new QLabel(name);
    lbl->setObjectName("sect");
    head->addWidget(lbl);
    head->addWidget(fan);
    head->addStretch();
    head->addWidget(temp);
    wl->addLayout(head);
    wl->addWidget(bar);
    return w;
}

static QProgressBar *makeBar(QWidget *parent)
{
    auto *b = new QProgressBar(parent);
    b->setRange(0, 100);
    b->setValue(0);
    b->setFormat(QString());
    b->setProperty("tier", 0);
    return b;
}

Popup::Popup(QSystemTrayIcon *tray, std::function<void()> openRgb, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_tray(tray), m_openRgb(std::move(openRgb))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(theme::stylesheet());
    setFixedWidth(300);

    auto *panel = new QFrame(this);
    panel->setObjectName("panel");

    // header
    auto *hdr = new QHBoxLayout;
    auto *dot = new QLabel(QStringLiteral("●"));
    dot->setObjectName("dot");
    auto *title = new QLabel("Nitro AN515-58");
    title->setObjectName("title");
    m_badge = new QLabel("…");
    m_badge->setObjectName("badge");
    auto *x = new QPushButton("×");
    x->setObjectName("x");
    connect(x, &QPushButton::clicked, this, &QWidget::hide);
    hdr->addWidget(dot);
    hdr->addWidget(title);
    hdr->addWidget(m_badge);
    hdr->addStretch();
    hdr->addWidget(x);

    // temp rows
    m_cpuFan = new QLabel("—"); m_cpuFan->setObjectName("fan");
    m_gpuFan = new QLabel("—"); m_gpuFan->setObjectName("fan");
    m_cpuTemp = new QLabel("--°C"); m_cpuTemp->setObjectName("temp");
    m_gpuTemp = new QLabel("--°C"); m_gpuTemp->setObjectName("temp");
    m_cpuBar = makeBar(this);
    m_gpuBar = makeBar(this);

    // load line
    auto *loadcard = new QFrame;
    loadcard->setObjectName("loadcard");
    auto *ll = new QHBoxLayout(loadcard);
    ll->setContentsMargins(10, 6, 8, 6);
    m_load = new QLabel("Load: …");
    m_load->setObjectName("load");
    auto *chip = new QLabel("≥88°");
    chip->setObjectName("chip");
    ll->addWidget(m_load);
    ll->addStretch();
    ll->addWidget(chip);

    // level buttons (3 per row)
    auto *btnGrid = new QVBoxLayout;
    btnGrid->setSpacing(7);
    for (int rowStart = 0; rowStart < 6; rowStart += 3) {
        auto *line = new QHBoxLayout;
        line->setSpacing(7);
        for (int i = rowStart; i < rowStart + 3 && i < 6; ++i) {
            const auto &lv = cfg::LEVELS[i];
            auto *b = new LevelButton(lv.key, lv.title, lv.sub);
            connect(b, &LevelButton::clicked, this, [this](const QString &k) {
                control::setLevel(k);
                refresh();
            });
            m_btns[QString(lv.key)] = b;
            line->addWidget(b);
        }
        btnGrid->addLayout(line);
    }

    // rgb button
    auto *rgbBtn = new QPushButton("Keyboard RGB");
    rgbBtn->setObjectName("rgbBtn");
    connect(rgbBtn, &QPushButton::clicked, this, [this] { m_openRgb(); });

    // footer
    auto *foot = new QHBoxLayout;
    const struct { const char *t; const char *obj; } footParts[] = {
        {"hot alert", "foot"}, {"≥88°", "foot"}, {"· guard:", "foot"},
        {"nitro-thermal", "footG"},
    };
    for (const auto &fp : footParts) {
        auto *l = new QLabel(fp.t);
        l->setObjectName(fp.obj);
        foot->addWidget(l);
    }
    foot->addStretch();

    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(11);
    lay->addLayout(hdr);
    lay->addWidget(makeHLine(this));
    lay->addWidget(tempRow("CPU", m_cpuFan, m_cpuTemp, m_cpuBar, this));
    lay->addWidget(tempRow("GPU", m_gpuFan, m_gpuTemp, m_gpuBar, this));
    lay->addWidget(loadcard);
    lay->addLayout(btnGrid);
    lay->addWidget(rgbBtn);
    lay->addLayout(foot);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    m_timer = new QTimer(this);
    m_timer->setInterval(cfg::POLL_MS);
    connect(m_timer, &QTimer::timeout, this, &Popup::refresh);
}

void Popup::toggle()
{
    if (isVisible()) {
        hide();
        return;
    }
    refresh();
    place();
    show();
    m_timer->start();
}

void Popup::place()
{
    const QRect geo = m_tray->geometry();
    QScreen *screen = QGuiApplication::screenAt(geo.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect sg = screen->availableGeometry();
    adjustSize();
    int x = geo.center().x() - width() / 2;
    x = qBound(sg.x() + 4, x, sg.x() + sg.width() - width() - 4);
    int y = geo.y() - height() - 8;
    if (y < sg.y() + 4)
        y = geo.bottom() + 8;
    move(x, y);
}

void Popup::refresh()
{
    const auto [cpu, gpu] = sensors::readTemps();
    const QString lvl = control::readLevel();
    const auto fans = sensors::readFans();

    QProgressBar *bars[2] = {m_cpuBar, m_gpuBar};
    const int vals[2] = {cpu, gpu};
    for (int i = 0; i < 2; ++i) {
        const int tier = theme::barTier(vals[i]);
        bars[i]->setValue(vals[i]);
        if (bars[i]->property("tier").toInt() != tier) {
            bars[i]->setProperty("tier", tier);
            bars[i]->style()->unpolish(bars[i]);
            bars[i]->style()->polish(bars[i]);
        }
    }
    m_cpuTemp->setText(QString("%1°C").arg(cpu));
    m_gpuTemp->setText(QString("%1°C").arg(gpu));
    m_badge->setText(lvl == "A" ? "auto"
                    : lvl == "?" ? lvl : "lvl" + lvl);

    if (fans.size() >= 2) {
        m_cpuFan->setText(QString("(%1%→%2%)").arg(fans[0].cur, 0, 'f', 0)
                              .arg(fans[0].tgt, 0, 'f', 0));
        m_gpuFan->setText(QString("(%1%→%2%)").arg(fans[1].cur, 0, 'f', 0)
                              .arg(fans[1].tgt, 0, 'f', 0));
        m_load->setText(QString("CPU %1%→%2% · GPU %3%→%4%")
                            .arg(fans[0].cur, 0, 'f', 0).arg(fans[0].tgt, 0, 'f', 0)
                            .arg(fans[1].cur, 0, 'f', 0).arg(fans[1].tgt, 0, 'f', 0));
    } else {
        m_cpuFan->setText("(n/a)");
        m_gpuFan->setText("(n/a)");
        m_load->setText("Load: n/a");
    }
    for (auto it = m_btns.begin(); it != m_btns.end(); ++it)
        it.value()->setActive(it.key() == lvl);
}

void Popup::hideEvent(QHideEvent *ev)
{
    m_timer->stop();
    QWidget::hideEvent(ev);
}

void Popup::keyPressEvent(QKeyEvent *ev)
{
    const QString t = ev->text();
    if (t.size() == 1 && t.at(0).isDigit() && t != "0") {
        control::setLevel(t);
        refresh();
        return;
    }
    if (t.compare("a", Qt::CaseInsensitive) == 0) {
        control::setLevel("A");
        refresh();
        return;
    }
    if (ev->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QWidget::keyPressEvent(ev);
}
