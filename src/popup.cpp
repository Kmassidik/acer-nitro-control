#include "popup.h"
#include "config.h"
#include "control.h"
#include "levelbutton.h"
#include "sensors.h"
#include "theme.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QColor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace {
QString repeat(const QString &s, int n) { return s.repeated(qMax(0, n)); }

QString fmtRpm(int v)
{
    QString s = QString::number(qMax(0, v));
    for (int i = s.size() - 3; i > 0; i -= 3)
        s.insert(i, ',');
    s = s.rightJustified(5, ' ');
    s.replace(' ', QStringLiteral("&#160;"));
    return s;
}

QString fmtTemp(int v, const QColor &col)
{
    QString s = QString("%1").arg(v, 3, 10, QLatin1Char(' '));
    s.replace(' ', QStringLiteral("&#160;"));
    return QString("<span style='color:%1'>%2°C</span>")
        .arg(col.name(), s);
}

QString tempBar(int v)
{
    const int filled = qBound(0, v, 100) * 20 / 100;
    return QString("<span style='color:%1'>%2</span><span style='color:%3'>%4</span>")
        .arg(theme::tempColor(v), repeat("█", filled),
             "#313244", repeat("░", 20 - filled));
}

QString rpmBar(int v, int max)
{
    const int filled = qBound(0, v, max) * 12 / max;
    return QString("<span style='color:%1'>%2</span><span style='color:%3'>%4</span>")
        .arg("#94e2d5", repeat("▮", filled),
             "#313244", repeat("▯", 12 - filled));
}

QString modeAliasFor(const QString &key)
{
    if (key == "A") return "auto";
    if (key == "1") return "chill";
    if (key == "2") return "cool";
    if (key == "3") return "game";
    if (key == "4") return "fast";
    if (key == "5") return "max";
    return "?";
}

QString modeSubFor(const QString &key)
{
    if (key == "A") return "Dynamic";
    for (const auto &lv : cfg::LEVELS)
        if (key == lv.key) return lv.sub;
    return "?";
}

void setPropertyActive(QWidget *w, bool on)
{
    w->setProperty("active", on);
    w->style()->unpolish(w);
    w->style()->polish(w);
}

bool isInteractiveControl(QObject *o)
{
    for (; o; o = o->parent()) {
        if (const auto *w = qobject_cast<const QWidget *>(o)) {
            if (qobject_cast<const QAbstractButton *>(w) ||
                qobject_cast<const QAbstractSlider *>(w) ||
                w->objectName() == "lvlBtn" ||
                w->objectName() == "swatch")
                return true;
        }
    }
    return false;
}
} // namespace

static QFrame *makeHLine(QWidget *parent)
{
    auto *ln = new QFrame(parent);
    ln->setFrameShape(QFrame::HLine);
    ln->setFixedHeight(1);
    ln->setStyleSheet("background: rgba(255,255,255,0.06); border: none;");
    return ln;
}

Popup::Popup(QSystemTrayIcon *tray, std::function<void()> openRgb, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_tray(tray), m_openRgb(std::move(openRgb))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(theme::stylesheet());
    setMinimumWidth(360);

    auto *panel = new QFrame(this);
    panel->setObjectName("panel");

    // custom controls — no native header/decorations
    auto *controls = new QHBoxLayout;
    controls->addStretch();
    auto *min = new QPushButton("–");
    min->setObjectName("x");
    auto *max = new QPushButton("□");
    max->setObjectName("x");
    auto *close = new QPushButton("×");
    close->setObjectName("x");
    connect(min, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(max, &QPushButton::clicked, this, &Popup::toggleMax);
    connect(close, &QPushButton::clicked, this, &QWidget::hide);
    controls->addWidget(min);
    controls->addWidget(max);
    controls->addWidget(close);

    // prompt
    auto *prompt = new QLabel;
    prompt->setObjectName("term");
    prompt->setText(
        "<span style='color:#6c7086'>~/AN515-58</span> "
        "<span style='color:#a6e3a1'>❯</span> "
        "<span style='color:#89b4fa'>thermal --watch</span>");
    prompt->setTextFormat(Qt::RichText);

    // temp/fan blocks
    m_cpuBlock = new QLabel;
    m_cpuBlock->setObjectName("term");
    m_cpuBlock->setTextFormat(Qt::RichText);
    m_gpuBlock = new QLabel;
    m_gpuBlock->setObjectName("term");
    m_gpuBlock->setTextFormat(Qt::RichText);

    auto *modeTitle = new QLabel(QStringLiteral("fan --mode (press 1-6)"));
    modeTitle->setObjectName("term");
    modeTitle->setStyleSheet("color:#6c7086;");

    // mode rows
    auto *modes = new QVBoxLayout;
    modes->setSpacing(2);
    for (const auto &lv : cfg::LEVELS) {
        const QString alias = modeAliasFor(lv.key);
        auto *b = new LevelButton(alias, alias, modeSubFor(lv.key));
        connect(b, &LevelButton::clicked, this, [this](const QString &k) {
            const QString key = (k == "auto") ? "A" : (k == "chill" ? "1" :
                               (k == "cool" ? "2" : (k == "game" ? "3" :
                               (k == "fast" ? "4" : "5"))));
            m_pendingLevel = key;
            updateModeUi(m_level);
        });
        m_btns[alias] = b;
        modes->addWidget(b);
    }

    m_modeStatus = new QLabel;
    m_modeStatus->setObjectName("load");
    m_modeStatus->setText("active: ?");

    m_applyBtn = new QPushButton("Apply");
    m_applyBtn->setObjectName("applyBtn");
    connect(m_applyBtn, &QPushButton::clicked, this, [this] {
        if (!m_pendingLevel.isEmpty()) {
            const QString applied = m_pendingLevel;
            m_applyBtn->setText(QString("Applying %1…").arg(modeAliasFor(applied)));
            m_applyBtn->setEnabled(false);
            m_modeStatus->setText(QString("applying: %1…").arg(modeAliasFor(applied)));
            repaint();
            control::setLevel(applied);
            m_pendingLevel.clear();
            m_applyBtn->setEnabled(true);
            refresh();
        }
    });

    auto *foot = new QLabel;
    foot->setObjectName("term");
    foot->setText("<span style='color:#6c7086'>alert ≥88° · guard nitro-thermal</span>");
    foot->setTextFormat(Qt::RichText);

    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(12, 10, 12, 12);
    lay->setSpacing(8);
    lay->addLayout(controls);
    lay->addWidget(prompt);
    lay->addSpacing(2);
    lay->addWidget(m_cpuBlock);
    lay->addWidget(m_gpuBlock);
    lay->addWidget(modeTitle);
    lay->addLayout(modes);
    lay->addWidget(m_modeStatus);
    lay->addWidget(m_applyBtn);
    lay->addWidget(foot);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    // Drag from any non-interactive area (labels/background), not from
    // buttons, sliders, swatches or mode rows.
    for (QWidget *child : findChildren<QWidget *>())
        child->installEventFilter(this);
    installEventFilter(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &Popup::refresh);

    m_anim = new QTimer(this);
    m_anim->setInterval(70);
    connect(m_anim, &QTimer::timeout, this, [this] {
        // smooth UI rpm (<=300 per frame) and advance spinner phase
        const int stepC = qBound(-300, m_targetCpuRpm - m_uiCpuRpm, 300);
        const int stepG = qBound(-300, m_targetGpuRpm - m_uiGpuRpm, 300);
        m_uiCpuRpm = qMax(0, m_uiCpuRpm + stepC);
        m_uiGpuRpm = qMax(0, m_uiGpuRpm + stepG);
        if (m_uiCpuRpm >= 50) m_cpuPhase += m_uiCpuRpm / 2200.0;
        if (m_uiGpuRpm >= 50) m_gpuPhase += m_uiGpuRpm / 2200.0;
        updateBlocks();
    });
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
    m_anim->start();
}

bool Popup::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type type = event->type();
    if (!m_maximized && (type == QEvent::MouseButtonPress ||
                         type == QEvent::MouseMove ||
                         type == QEvent::MouseButtonRelease)) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (!isInteractiveControl(watched)) {
            if (type == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
                if (QWindow *h = windowHandle()) {
                    h->startSystemMove();
                } else {
                    m_dragging = true;
                    m_dragPos = me->globalPosition().toPoint() - frameGeometry().topLeft();
                }
                return false;
            }
            if (type == QEvent::MouseMove && m_dragging && (me->buttons() & Qt::LeftButton)) {
                move(me->globalPosition().toPoint() - m_dragPos);
                return true;
            }
            if (type == QEvent::MouseButtonRelease)
                m_dragging = false;
        } else if (type == QEvent::MouseButtonPress) {
            m_dragging = false;
        }
    }
    return QWidget::eventFilter(watched, event);
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

void Popup::updateBlocks()
{
    const QColor c1 = QColor(theme::tempColor(m_cpuTempVal));
    const QColor c2 = QColor(theme::tempColor(m_gpuTempVal));
    const bool cpuStalled = (m_uiCpuRpm == 0 && m_level != "1" && m_level != "A");
    const bool gpuStalled = (m_uiGpuRpm == 0 && m_level != "1" && m_level != "A");

    auto blockHTML = [&](const QString &name, int t, int rpm, double phase, bool stalled, const QColor &col) {
        const QString tb = tempBar(t);
        const QString rb = rpmBar(stalled ? 0 : rpm, 8500);
        QString line2 = QString("&#160;&#160;<span style='color:#6c7086'>fan</span> ");
        if (stalled)
            line2 += "<span style='color:#f38ba8'>stalled</span> <span style='color:#6c7086'>rpm</span> <span style='color:#313244'>" + repeat("▯", 12) + "</span>";
        else
            line2 += QString("<span style='color:#89b4fa'>%1</span> <span style='color:#6c7086'>rpm</span> <span style='color:#94e2d5'>%2</span>").arg(fmtRpm(rpm), rb);
        return QString("<span style='color:#6c7086'>%1&nbsp;</span>"
                       "<span style='color:%2'>%3°C</span> "
                       "<span style='color:%2'>%4</span><br>%5")
            .arg(name, col.name(),
                 QString::number(t).rightJustified(3, ' ').replace(' ', QStringLiteral("&#160;")),
                 tb, line2);
    };
    Q_UNUSED(blockHTML);

    // For simplicity, build with explicit HTML strings.
    QString cpu1 = QString("<span style='color:#6c7086'>cpu&nbsp;</span>"
                           "<span style='color:%1'>%2°C</span> "
                           "<span style='color:%1'>%3</span><br>")
                       .arg(c1.name(),
                            QString::number(m_cpuTempVal).rightJustified(3, ' ').replace(' ', QStringLiteral("&#160;")),
                            tempBar(m_cpuTempVal));
    const QString cpuPrefix = cpuStalled
        ? QStringLiteral("<span style='color:#f38ba8'>stalled</span>&#160;")
        : QStringLiteral("&#160;&#160;");
    QString cpu2 = QString("%1<span style='color:#6c7086'>fan</span> ").arg(cpuPrefix);
    if (cpuStalled)
        cpu2 += QString("<span style='color:#6c7086'>rpm</span> <span style='color:#313244'>%1</span>")
                    .arg(rpmBar(0, 8500));
    else
        cpu2 += QString("<span style='color:#89b4fa'>%1</span> <span style='color:#6c7086'>rpm</span> <span style='color:#94e2d5'>%2</span>")
                    .arg(fmtRpm(m_uiCpuRpm), rpmBar(m_uiCpuRpm, 8500));
    m_cpuBlock->setText(cpu1 + cpu2);

    QString gpu1 = QString("<span style='color:#6c7086'>gpu&nbsp;</span>"
                           "<span style='color:%1'>%2°C</span> "
                           "<span style='color:%1'>%3</span><br>")
                       .arg(c2.name(),
                            QString::number(m_gpuTempVal).rightJustified(3, ' ').replace(' ', QStringLiteral("&#160;")),
                            tempBar(m_gpuTempVal));
    const QString gpuPrefix = gpuStalled
        ? QStringLiteral("<span style='color:#f38ba8'>stalled</span>&#160;")
        : QStringLiteral("&#160;&#160;");
    QString gpu2 = QString("%1<span style='color:#6c7086'>fan</span> ").arg(gpuPrefix);
    if (gpuStalled)
        gpu2 += QString("<span style='color:#6c7086'>rpm</span> <span style='color:#313244'>%1</span>")
                    .arg(rpmBar(0, 8500));
    else
        gpu2 += QString("<span style='color:#89b4fa'>%1</span> <span style='color:#6c7086'>rpm</span> <span style='color:#94e2d5'>%2</span>")
                    .arg(fmtRpm(m_uiGpuRpm), rpmBar(m_uiGpuRpm, 8500));
    m_gpuBlock->setText(gpu1 + gpu2);
}

void Popup::refresh()
{
    const auto [cpu, gpu] = sensors::readTemps();
    const QString lvl = control::readLevel();
    const auto fans = sensors::readFans();

    m_cpuTempVal = cpu;
    m_gpuTempVal = gpu;
    m_level = lvl;

    if (fans.size() >= 2) {
        const double c0 = fans[0].steps > 0 ? fans[0].steps : 6000;
        const double g0 = fans[1].steps > 0 ? fans[1].steps : 6000;
        m_targetCpuRpm = qRound(fans[0].cur / 100.0 * c0);
        m_targetGpuRpm = qRound(fans[1].cur / 100.0 * g0);
    } else {
        m_targetCpuRpm = 0;
        m_targetGpuRpm = 0;
    }

    const QString shownLevel = !m_pendingLevel.isEmpty() ? m_pendingLevel : lvl;
    updateModeUi(shownLevel);
    updateBlocks();
}

void Popup::updateModeUi(const QString &shownLevel)
{
    const QString alias = modeAliasFor(shownLevel);
    for (auto it = m_btns.begin(); it != m_btns.end(); ++it)
        it.value()->setActive(it.key() == alias);
    if (!m_pendingLevel.isEmpty()) {
        m_applyBtn->setText(QString("Apply %1").arg(modeAliasFor(m_pendingLevel)));
        m_modeStatus->setText(QString("selected: %1 — press Apply").arg(modeAliasFor(m_pendingLevel)));
    } else {
        m_applyBtn->setText("Apply");
        m_modeStatus->setText(QString("active: %1").arg(alias));
    }
}

void Popup::toggleMax()
{
    if (m_maximized) {
        setGeometry(m_normalGeo);
        m_maximized = false;
    } else {
        m_normalGeo = geometry();
        QScreen *screen = QGuiApplication::screenAt(geometry().center());
        if (!screen) screen = QGuiApplication::primaryScreen();
        setGeometry(screen->availableGeometry());
        m_maximized = true;
    }
}

void Popup::hideEvent(QHideEvent *ev)
{
    m_timer->stop();
    m_anim->stop();
    QWidget::hideEvent(ev);
}

void Popup::keyPressEvent(QKeyEvent *ev)
{
    const QString t = ev->text();
    if (t == "1") { m_pendingLevel = "1"; updateModeUi("1"); return; }
    if (t == "2") { m_pendingLevel = "2"; updateModeUi("2"); return; }
    if (t == "3") { m_pendingLevel = "3"; updateModeUi("3"); return; }
    if (t == "4") { m_pendingLevel = "4"; updateModeUi("4"); return; }
    if (t == "5") { m_pendingLevel = "5"; updateModeUi("5"); return; }
    if (t == "6" || t.compare("a", Qt::CaseInsensitive) == 0) { m_pendingLevel = "A"; updateModeUi("A"); return; }
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
        if (!m_pendingLevel.isEmpty()) {
            const QString applied = m_pendingLevel;
            m_applyBtn->setText(QString("Applying %1…").arg(modeAliasFor(applied)));
            m_applyBtn->setEnabled(false);
            m_modeStatus->setText(QString("applying: %1…").arg(modeAliasFor(applied)));
            repaint();
            control::setLevel(applied);
            m_pendingLevel.clear();
            m_applyBtn->setEnabled(true);
            refresh();
        }
        return;
    }
    if (ev->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QWidget::keyPressEvent(ev);
}

void Popup::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = ev->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(ev);
}

void Popup::mouseMoveEvent(QMouseEvent *ev)
{
    if (m_dragging)
        move(ev->globalPosition().toPoint() - m_dragPos);
    QWidget::mouseMoveEvent(ev);
}

void Popup::mouseReleaseEvent(QMouseEvent *ev)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(ev);
}
