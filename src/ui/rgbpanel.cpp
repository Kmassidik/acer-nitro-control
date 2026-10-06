// Glass Ghost RGB panel — live animated keyboard preview, 4 zones,
// effects, palette, brightness/speed, link toggle. Real device writes on Apply.
#include "ui/rgbpanel.h"
#include "core/config.h"
#include "core/control.h"
#include "ui/segmented.h"
#include "ui/theme.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QColorDialog>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <cmath>

namespace {

constexpr int KB_COLS = 15;
constexpr int KB_ROWS = 5;

const struct { const char *k, *t, *s; } FX_MODES[4] = {
    {"0", "Static", ""}, {"1", "Breathe", ""}, {"2", "Wave", ""}, {"3", "Neon", ""},
};

// Glass Ghost palette (mock's PAL), hex strings
const char *PALETTE[] = {
    "#a78bfa", "#60a5fa", "#22d3ee", "#34d399", "#fbbf24",
    "#fb923c", "#f472b6", "#f87171", "#f5f5f5",
};

QString defaultZoneHex(int i)
{
    static const char *defs[4] = {"#a78bfa", "#60a5fa", "#34d399", "#fbbf24"};
    return QLatin1String(defs[i]);
}

QJsonArray rgbOf(const QColor &c) { return QJsonArray{c.red(), c.green(), c.blue()}; }

QColor zoneColor(const QJsonObject &st, int i)
{
    const QJsonArray zones = st["zones"].toArray();
    if (i < zones.size() && zones[i].isArray()) {
        const QJsonArray z = zones[i].toArray();
        if (z.size() >= 3)
            return QColor(z[0].toInt(), z[1].toInt(), z[2].toInt());
    }
    return QColor(defaultZoneHex(i));
}

// hsv(h in deg) -> rgb ints (same helper as the mock)
QColor hsv(int h)
{
    h = ((h % 360) + 360) % 360;
    auto f = [&](int n) -> int {
        const int k = (n + h / 60) % 6;
        const double v = 1 - std::max(0, std::min(std::min(k, 4 - k), 1));
        return qRound(255 * v);
    };
    return QColor(f(5), f(3), f(1));
}

} // namespace

// ---------------- Keycap ----------------
Keycap::Keycap(int row, int col, QWidget *parent)
    : QWidget(parent), m_row(row), m_col(col)
{
    setFixedSize(14, 14);
    setCursor(Qt::PointingHandCursor);
}

void Keycap::setRgb(const QColor &c, double glow)
{
    m_c = c;
    m_glow = glow;
    update();
}

void Keycap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(m_c);
    p.drawRoundedRect(rect(), 3, 3);
    if (m_glow > 0.05) {
        p.setPen(QPen(QColor(255, 255, 255, int(255 * 0.4 * m_glow)), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect(), 3, 3);
    }
}

void Keycap::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit picked(m_row, m_col);
    QWidget::mousePressEvent(ev);
}

// ---------------- RgbPanel ----------------
RgbPanel::RgbPanel(QWidget *parent, std::function<void()> openPopup)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_openPopup(std::move(openPopup))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(theme::stylesheet());
    setMinimumWidth(356);

    auto *panel = new QFrame(this);
    panel->setObjectName("panel");
    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(22, 10, 22, 22);
    lay->setSpacing(6);

    // titlebar: utility buttons (Option 2) + centered title
    auto *dots = new QHBoxLayout;
    dots->setSpacing(7);
    auto mkBtn = [this](std::function<void()> act, const QString &label) {
        auto *d = new QPushButton(this);
        d->setObjectName("utilBtn");
        d->setFixedHeight(24);
        d->setCursor(Qt::PointingHandCursor);
        d->setToolTip(label);
        d->setAttribute(Qt::WA_LayoutOnEntireRect);
        d->installEventFilter(this);
        m_dotHints[d] = label;
        connect(d, &QPushButton::clicked, this, [this, act] {
            if (act)                    // INVARIANT: empty fn must never fire
                act();
        });
        return d;
    };
    auto *fanBtn = mkBtn([this] {
        if (m_openPopup)
            m_openPopup();
    }, "Fan panel");
    fanBtn->setText("Fan");
    dots->addWidget(fanBtn);
    dots->addSpacing(10);
    m_baseTitle = QStringLiteral("Keyboard RGB");
    auto *ttl = new QLabel(m_baseTitle);
    ttl->setObjectName("ttlc");
    ttl->setAlignment(Qt::AlignCenter);
    dots->addWidget(ttl, 1);
    dots->addSpacing(12);
    m_title = ttl;

    // keyboard preview
    m_kb = new QWidget;
    m_kb->setObjectName("kb");
    auto *kgrid = new QGridLayout(m_kb);
    kgrid->setContentsMargins(10, 10, 10, 10);
    kgrid->setSpacing(3);
    m_keys.resize(KB_ROWS);
    for (int r = 0; r < KB_ROWS; ++r) {
        m_keys[r].reserve(KB_COLS);
        for (int c = 0; c < KB_COLS; ++c) {
            auto *k = new Keycap(r, c, m_kb);
            k->setObjectName("keycap");
            k->setZone(int(double(c) / KB_COLS * 4));   // := mock mapping
            connect(k, &Keycap::picked, this, [this](int row, int col) {
                selectZone(m_keys[row][col]->zone());
            });
            kgrid->addWidget(k, r, c);
            m_keys[r].append(k);
        }
    }

    // effects segmented
    m_fx = new Segmented;
    QStringList fxLabels;
    for (const auto &m : FX_MODES)
        fxLabels << m.t;
    m_fx->setOptions(fxLabels);
    m_fx->setPillColor(QColor(255, 255, 255, 28));
    m_fx->setAccentColor(Qt::white);
    connect(m_fx, &Segmented::selected, this, [this](int idx) {
        m_state["mode"] = idx;
        syncUiFromState();
    });

    // zone row
    auto *zlab = new QLabel("ZONE");
    zlab->setObjectName("lab");
    m_zoneLbl = new QLabel("Zone 1");
    m_zoneLbl->setObjectName("lab");
    auto *zrow = new QHBoxLayout;
    zrow->addWidget(zlab);
    zrow->addStretch();
    zrow->addWidget(m_zoneLbl);
    auto *zones = new QHBoxLayout;
    zones->setSpacing(8);
    for (int i = 0; i < 4; ++i) {
        auto *b = new QPushButton;
        b->setObjectName("zone");
        b->setProperty("sel", i == 0);
        b->setText(QString::number(i + 1));
        b->setFixedHeight(38);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, i] { selectZone(i); });
        m_zones.append(b);
        zones->addWidget(b, 1);
    }

    // palette row
    auto *pal = new QHBoxLayout;
    pal->setSpacing(4);
    for (const char *hex : PALETTE) {
        auto *b = new QPushButton;
        b->setObjectName("palBtn");
        b->setStyleSheet(QString("background: %1;").arg(hex));
        b->setCursor(Qt::PointingHandCursor);
        const QColor c(hex);
        connect(b, &QPushButton::clicked, this, [this, c] {
            if (m_link->isChecked())
                for (int i = 0; i < 4; ++i) {
                    m_state["zones"] = QJsonArray{};
                    QJsonArray zones = m_state["zones"].toArray();
                    zones.append(rgbOf(c));
                    m_state["zones"] = zones;
                    m_zones[i]->setStyleSheet(
                        m_zones[i]->styleSheet().replace(
                            QRegularExpression("background:[^;]+"),
                            QString("background: %1").arg(c.name())));
                }
            else {
                QJsonArray zones = m_state["zones"].toArray();
                while (zones.size() < 4)
                    zones.append(rgbOf(QColor(defaultZoneHex(zones.size()))));
                zones[m_selZone] = rgbOf(c);
                m_state["zones"] = zones;
            }
            syncUiFromState();
            apply();
        });
        m_palBtns.append(b);
        pal->addWidget(b);
    }
    pal->addStretch();

    // custom pick button appended to palette
    auto *pick = new QPushButton("+");
    pick->setObjectName("palBtn");
    pick->setStyleSheet("background: rgba(255,255,255,0.08); color: #e8e6f5;");
    pick->setCursor(Qt::PointingHandCursor);
    connect(pick, &QPushButton::clicked, this, [this] {
        const QColor c = QColorDialog::getColor(
            zoneColor(m_state, m_selZone), this, "Zone color");
        if (!c.isValid())
            return;
        QJsonArray zones = m_state["zones"].toArray();
        while (zones.size() < 4)
            zones.append(rgbOf(QColor(defaultZoneHex(zones.size()))));
        if (m_link->isChecked()) {
            for (int i = 0; i < 4; ++i)
                zones[i] = rgbOf(c);
        } else {
            zones[m_selZone] = rgbOf(c);
        }
        m_state["zones"] = zones;
        syncUiFromState();
        apply();
    });
    pal->addWidget(pick);

    // brightness + speed
    auto mkSlider = [this](const char *labelTxt, QSlider *&s, QLabel *&val,
                           int lo, int hi, const char *stateKey) {
        auto *lab = new QLabel(labelTxt);
        lab->setObjectName("lab");
        val = new QLabel;
        val->setObjectName("lab");
        s = new QSlider(Qt::Horizontal);
        s->setRange(lo, hi);
        connect(s, &QSlider::valueChanged, this, [this, val, stateKey](int v) {
            val->setText(QString("%1%").arg(v));
            m_state[stateKey] = v;
            paintKeyboard();
        });
        auto *h = new QHBoxLayout;
        h->addWidget(lab);
        h->addStretch();
        h->addWidget(val);
        auto *box = new QWidget;
        auto *v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
        v->addLayout(h);
        v->addWidget(s);
        return box;
    };
    auto *brightBox = mkSlider("BRIGHTNESS", m_bright, m_brightVal, 0, 100, "brightness");
    auto *speedBox = mkSlider("SPEED", m_speed, m_speedVal, 5, 100, "speed");

    // link + apply
    auto *hline2 = new QFrame(this);
    hline2->setObjectName("hline");
    hline2->setFixedHeight(1);
    auto *rowL = new QHBoxLayout;
    rowL->setContentsMargins(2, 9, 2, 9);
    auto *rl = new QLabel("Link all zones");
    rl->setObjectName("row");
    m_link = new Toggle;
    m_link->setProperty("linkToggle", true);
    connect(m_link, &Toggle::toggled, this, [this](bool on) {
        m_state["sync"] = on;
        if (on) {
            QJsonArray zones;
            for (int i = 0; i < 4; ++i)
                zones.append(rgbOf(zoneColor(m_state, m_selZone)));
            m_state["zones"] = zones;
        }
        syncUiFromState();
    });
    rowL->addWidget(rl);
    rowL->addStretch();
    rowL->addWidget(m_link);
    auto *rowLw = new QWidget;
    rowLw->setLayout(rowL);

    auto *applyBtn = new QPushButton("Apply");
    applyBtn->setObjectName("applyBtn");
    applyBtn->setCursor(Qt::PointingHandCursor);
    connect(applyBtn, &QPushButton::clicked, this, &RgbPanel::apply);
    m_status = new QLabel("ready");
    m_status->setObjectName("status");

    lay->addLayout(dots);
    lay->addSpacing(4);
    lay->addWidget(m_kb);
    lay->addSpacing(4);
    lay->addWidget(m_fx);
    lay->addSpacing(4);
    lay->addLayout(zrow);
    lay->addLayout(zones);
    lay->addSpacing(2);
    lay->addLayout(pal);
    lay->addSpacing(6);
    lay->addWidget(brightBox);
    lay->addWidget(speedBox);
    lay->addWidget(hline2);
    lay->addWidget(rowLw);
    lay->addSpacing(4);
    lay->addWidget(applyBtn);
    lay->addWidget(m_status);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    // effect animation — only while visible
    m_paintTimer = new QTimer(this);
    m_paintTimer->setInterval(50);
    connect(m_paintTimer, &QTimer::timeout, this, [this] {
        m_t += 0.05 * (0.3 + m_state["speed"].toInt() / 100.0 * 2.2);
        paintKeyboard();
    });
    m_paintTimer->start();

    // drag from non-interactive areas
    for (QWidget *child : findChildren<QWidget *>())
        child->installEventFilter(this);
    installEventFilter(this);
}

void RgbPanel::showEvent(QShowEvent *ev)
{
    m_state = control::loadRgb();
    loadUi();
    m_paintTimer->start();
    QWidget::showEvent(ev);
}

void RgbPanel::hideEvent(QHideEvent *ev)
{
    m_paintTimer->stop();
    QWidget::hideEvent(ev);
}

void RgbPanel::loadUi()
{
    syncUiFromState();
}

void RgbPanel::syncUiFromState()
{
    const int mode = m_state["mode"].toInt(0);
    m_fx->select(qBound(0, mode, 3), false);

    for (int i = 0; i < 4; ++i) {
        const QColor c = zoneColor(m_state, i);
        m_zones[i]->setStyleSheet(
            QString("background: %1;").arg(c.name()));
        m_zones[i]->setProperty("sel", i == m_selZone);
    }
    m_zoneLbl->setText(QString("Zone %1").arg(m_selZone + 1));

    m_bright->setValue(m_state["brightness"].toInt(80));
    m_speed->setValue(qBound(5, m_state["speed"].toInt(50), 100));
    m_brightVal->setText(QString("%1%").arg(m_bright->value()));
    m_speedVal->setText(QString("%1%").arg(m_speed->value()));
    m_link->setChecked(m_state["sync"].toBool());

    paintKeyboard();
}

void RgbPanel::selectZone(int idx)
{
    m_selZone = qBound(0, idx, 3);
    syncUiFromState();
}

void RgbPanel::paintKeyboard()
{
    const int mode = m_state["mode"].toInt(0);
    const double bri = m_bright->value() / 100.0;
    const QJsonArray zones = m_state["zones"].toArray();
    QColor zcol[4];
    for (int i = 0; i < 4; ++i)
        zcol[i] = (i < zones.size()) ? zoneColor(m_state, i)
                                      : QColor(defaultZoneHex(i));

    for (int r = 0; r < KB_ROWS; ++r) {
        for (int c = 0; c < KB_COLS; ++c) {
            auto *k = m_keys[r][c];
            const int z = k->zone();
            QColor col = zcol[z];
            double f = 1.0;
            if (mode == 1)          // Breathe
                f = 0.3 + 0.7 * (0.5 + 0.5 * std::sin(m_t * 2));
            else if (mode == 2)     // Wave
                f = 0.15 + 0.85 * (0.5 + 0.5 * std::sin(c * 0.55 - m_t * 4));
            else if (mode == 3)     // Neon
                col = hsv(m_t * 40 + z * 70 + c * 6);
            const double v = 0.1 + 0.9 * f * bri;
            QColor out(qRound(col.red() * v + 12 * (1 - v)),
                       qRound(col.green() * v + 12 * (1 - v)),
                       qRound(col.blue() * v + 12 * (1 - v)));
            k->setRgb(out, (bri > 0.05 && f > 0.5) ? f * bri : 0.0);
        }
    }
    // window glow tint follows the selected zone (mock --glow)
    setProperty("glowColor", zcol[m_selZone].name());
}

void RgbPanel::apply()
{
    m_state["brightness"] = m_bright->value();
    m_state["speed"] = m_speed->value();
    m_state["sync"] = m_link->isChecked();
    m_state["mode"] = m_fx->current();
    QString err;
    const bool ok = control::applyRgb(m_state, &err);
    m_status->setText(ok ? "✓ applied" : "✗ " + err);
}

// drag helpers (same strategy as Popup)
bool RgbPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (auto *btn = qobject_cast<QPushButton *>(watched)) {
        if (m_dotHints.contains(btn) && m_title) {
            if (event->type() == QEvent::Enter)
                m_title->setText(m_dotHints.value(btn));
            else if (event->type() == QEvent::Leave)
                m_title->setText(m_baseTitle);
        }
    }
    const QEvent::Type type = event->type();
    if (type == QEvent::MouseButtonPress || type == QEvent::MouseMove ||
        type == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        auto *w = qobject_cast<QWidget *>(watched);
        const bool interactive = w && (qobject_cast<QAbstractButton *>(w) ||
                                       qobject_cast<QAbstractSlider *>(w));
        if (!interactive) {
            if (type == QEvent::MouseButtonPress &&
                me->button() == Qt::LeftButton) {
                if (QWindow *h = windowHandle())
                    h->startSystemMove();
                else {
                    m_dragging = true;
                    m_dragPos = me->globalPosition().toPoint() -
                                frameGeometry().topLeft();
                }
                return false;
            }
            if (type == QEvent::MouseMove && m_dragging &&
                (me->buttons() & Qt::LeftButton)) {
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

void RgbPanel::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = ev->globalPosition().toPoint() -
                    frameGeometry().topLeft();
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
