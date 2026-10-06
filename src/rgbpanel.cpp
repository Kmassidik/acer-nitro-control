#include "rgbpanel.h"
#include "config.h"
#include "control.h"
#include "levelbutton.h"
#include "theme.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QColorDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

// ---------------- Swatch ----------------
Swatch::Swatch(QWidget *parent) : QFrame(parent)
{
    setObjectName("swatch");
    setCursor(Qt::PointingHandCursor);
    setColor(QColor("#cba6f7"));
}

void Swatch::setColor(const QColor &c)
{
    m_c = c;
    setStyleSheet(QString("background: %1; border-radius: 6px;"
                          "border: 1px solid rgba(255,255,255,0.14);")
                      .arg(c.name()));
}

void Swatch::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked();
    QFrame::mousePressEvent(ev);
}

// ---------------- RgbPanel ----------------
static const struct { const char *k, *t, *s; } MODES[6] = {
    {"0", "Static", "zones"}, {"1", "Breath", "pulse"}, {"2", "Neon", "cycle"},
    {"3", "Wave", "flow"},    {"4", "Shift", "glide"},  {"5", "Zoom", "zoom"},
};

static QFrame *makeHLine(QWidget *parent)
{
    auto *ln = new QFrame(parent);
    ln->setFrameShape(QFrame::HLine);
    ln->setFixedHeight(1);
    ln->setStyleSheet("background: rgba(255,255,255,0.06); border: none;");
    return ln;
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

RgbPanel::RgbPanel(QWidget *parent, std::function<void()> openPopup)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool), m_openPopup(std::move(openPopup))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(theme::stylesheet());
    setMinimumWidth(360);

    auto *panel = new QFrame(this);
    panel->setObjectName("panel");

    auto *controls = new QHBoxLayout;
    controls->addStretch();
    auto *min = new QPushButton("–");
    min->setObjectName("x");
    auto *max = new QPushButton("□");
    max->setObjectName("x");
    auto *close = new QPushButton("×");
    close->setObjectName("x");
    connect(min, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(max, &QPushButton::clicked, this, &RgbPanel::toggleMax);
    connect(close, &QPushButton::clicked, this, &QWidget::hide);
    controls->addWidget(min);
    controls->addWidget(max);
    controls->addWidget(close);

    // zones + sync
    auto *zrow = new QHBoxLayout;
    zrow->setSpacing(6);
    for (int i = 0; i < 4; ++i) {
        auto *wrap = new QVBoxLayout;
        wrap->setSpacing(2);
        m_zones[i] = new Swatch;
        connect(m_zones[i], &Swatch::clicked, this, [this, i] { pickZone(i); });
        auto *lbl = new QLabel(QString("Z%1").arg(i + 1));
        lbl->setObjectName("swLbl");
        lbl->setAlignment(Qt::AlignCenter);
        wrap->addWidget(m_zones[i]);
        wrap->addWidget(lbl);
        zrow->addLayout(wrap);
    }
    m_sync = new QPushButton("Sync");
    m_sync->setObjectName("chipBtn");
    m_sync->setCheckable(true);
    connect(m_sync, &QPushButton::toggled, this, [this](bool on) {
        if (on) {
            const QJsonArray z0 = m_state["zones"].toArray();
            QJsonArray zones;
            for (int i = 0; i < 4; ++i)
                zones.append(z0.isEmpty() ? QJsonArray{217, 168, 98} : z0[0]);
            m_state["zones"] = zones;
            for (auto *z : m_zones)
                z->setColor(QColor(z0.isEmpty() ? QColor("#cba6f7").name()
                                                : QColor(z0[0].toArray()[0].toInt(),
                                                         z0[0].toArray()[1].toInt(),
                                                         z0[0].toArray()[2].toInt()).name()));
        }
    });
    zrow->addWidget(m_sync, 0, Qt::AlignBottom);

    // fx color
    auto *frow = new QHBoxLayout;
    auto *flbl = new QLabel("FX color");
    flbl->setObjectName("swLbl");
    m_fx = new Swatch;
    connect(m_fx, &Swatch::clicked, this, &RgbPanel::pickFx);
    frow->addWidget(flbl);
    frow->addWidget(m_fx);
    frow->addStretch();

    // modes list
    auto *modes = new QVBoxLayout;
    modes->setSpacing(2);
    for (int i = 0; i < 6; ++i) {
        auto *b = new LevelButton(MODES[i].t, MODES[i].t, MODES[i].s);
        connect(b, &LevelButton::clicked, this, [this](const QString &title) {
            const QString modeKey = title;
            for (int j = 0; j < 6; ++j)
                if (QString::fromLatin1(MODES[j].t) == modeKey)
                    m_state["mode"] = j;
            for (auto it = m_modes.begin(); it != m_modes.end(); ++it)
                it.value()->setActive(it.key() == modeKey);
            m_status->setText(QString("mode=%1").arg(modeKey));
        });
        m_modes[QString(MODES[i].t)] = b;
        modes->addWidget(b);
    }

    // sliders
    auto sliderRow = [this](const char *name, QSlider *&s, QLabel *&val,
                            int lo, int hi) {
        auto *row = new QHBoxLayout;
        row->setSpacing(8);
        auto *lbl = new QLabel(name);
        lbl->setObjectName("sect");
        s = new QSlider(Qt::Horizontal);
        s->setRange(lo, hi);
        val = new QLabel;
        val->setObjectName("sliderVal");
        connect(s, &QSlider::valueChanged, val,
                [val](int v) { val->setText(QString::number(v)); });
        row->addWidget(lbl);
        row->addWidget(s, 1);
        row->addWidget(val);
        return row;
    };
    auto *speedRow = sliderRow("Speed", m_speed, m_speedVal, 1, 9);
    auto *brightRow = sliderRow("Brightness", m_bright, m_brightVal, 0, 100);

    // apply + status
    auto *applyBtn = new QPushButton("Apply");
    applyBtn->setObjectName("applyBtn");
    connect(applyBtn, &QPushButton::clicked, this, &RgbPanel::apply);
    m_status = new QLabel("ready");
    m_status->setObjectName("status");
    m_status->setTextFormat(Qt::RichText);
    m_status->setText("<span style='color:#6c7086'>ready</span>");

    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(12, 10, 12, 12);
    lay->setSpacing(10);
    lay->addLayout(controls);
    lay->addLayout(zrow);
    lay->addLayout(frow);
    lay->addLayout(modes);
    lay->addLayout(speedRow);
    lay->addLayout(brightRow);
    lay->addWidget(applyBtn);
    lay->addWidget(m_status);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    // Drag from any non-interactive area (labels/background), not from
    // buttons, sliders, swatches or mode rows.
    for (QWidget *child : findChildren<QWidget *>())
        child->installEventFilter(this);
    installEventFilter(this);
}

bool RgbPanel::eventFilter(QObject *watched, QEvent *event)
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

void RgbPanel::toggleMax()
{
    if (m_maximized) {
        setGeometry(m_normalGeo);
        m_maximized = false;
    } else {
        m_normalGeo = geometry();
        QScreen *screen = nullptr;
        for (QScreen *s : qApp->screens())
            if (s->availableGeometry().contains(geometry().center())) { screen = s; break; }
        if (!screen) screen = QGuiApplication::primaryScreen();
        setGeometry(screen->availableGeometry());
        m_maximized = true;
    }
}

void RgbPanel::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = ev->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(ev);
}

void RgbPanel::mouseMoveEvent(QMouseEvent *ev)
{
    if (m_dragging)
        move(ev->globalPosition().toPoint() - m_dragPos);
    QWidget::mouseMoveEvent(ev);
}

void RgbPanel::mouseReleaseEvent(QMouseEvent *ev)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(ev);
}

void RgbPanel::showEvent(QShowEvent *ev)
{
    m_state = control::loadRgb();
    loadUi();
    QWidget::showEvent(ev);
}

void RgbPanel::loadUi()
{
    const QJsonArray zones = m_state["zones"].toArray();
    for (int i = 0; i < 4; ++i) {
        const QJsonArray z = (i < zones.size() && zones[i].isArray())
                                 ? zones[i].toArray() : QJsonArray{217, 168, 98};
        m_zones[i]->setColor(QColor(z[0].toInt(217), z[1].toInt(168), z[2].toInt(98)));
    }
    const QJsonArray c = m_state["color"].isArray()
                             ? m_state["color"].toArray() : QJsonArray{217, 168, 98};
    m_fx->setColor(QColor(c[0].toInt(217), c[1].toInt(168), c[2].toInt(98)));
    m_sync->setChecked(m_state["sync"].toBool());
    const int mode = m_state["mode"].toInt(1);
    for (auto it = m_modes.begin(); it != m_modes.end(); ++it)
        it.value()->setActive(it.key().toInt() == mode || it.key() == QString::fromLatin1(MODES[mode].t));
    m_speed->setValue(m_state["speed"].toInt(4));
    m_bright->setValue(m_state["brightness"].toInt(100));
    m_speedVal->setText(QString::number(m_speed->value()));
    m_brightVal->setText(QString::number(m_bright->value()));
}

void RgbPanel::pickZone(int idx)
{
    const QColor c = QColorDialog::getColor(m_zones[idx]->color(), this,
                                            QString("Zone %1").arg(idx + 1));
    if (!c.isValid())
        return;
    QJsonArray zones = m_state["zones"].toArray();
    while (zones.size() < 4)
        zones.append(QJsonArray{217, 168, 98});
    const QJsonArray rgb{c.red(), c.green(), c.blue()};
    if (m_sync->isChecked()) {
        for (int i = 0; i < 4; ++i) {
            zones[i] = rgb;
            m_zones[i]->setColor(c);
        }
    } else {
        zones[idx] = rgb;
        m_zones[idx]->setColor(c);
    }
    m_state["zones"] = zones;
}

void RgbPanel::pickFx()
{
    const QColor c = QColorDialog::getColor(m_fx->color(), this, "FX color");
    if (!c.isValid())
        return;
    m_state["color"] = QJsonArray{c.red(), c.green(), c.blue()};
    m_fx->setColor(c);
}

void RgbPanel::apply()
{
    m_state["speed"] = m_speed->value();
    m_state["brightness"] = m_bright->value();
    m_state["sync"] = m_sync->isChecked();
    QString err;
    const bool ok = control::applyRgb(m_state, &err);
    m_status->setText(ok ? "✓ applied" : "✗ " + err);
}
