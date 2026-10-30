#include "ui/popup.h"
#include "core/config.h"
#include "core/control.h"
#include "core/sensors.h"
#include "ui/segmented.h"
#include "ui/theme.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace {

int nominalMaxRpm(const sensors::Fan &f)
{
    const double steps = f.steps > 0 ? f.steps : 6000.0;
    return qRound(f.cur / 100.0 * steps);
}

} // namespace

// ---------------- TempRing (Glass Ghost .ring port) ----------------
class TempRing : public QWidget
{
public:
    explicit TempRing(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedSize(170, 170);
    }
    void setTemp(int t)
    {
        if (t == m_t)
            return;
        m_t = t;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        // track
        QPen track(QColor(255, 255, 255, 16), 6, Qt::SolidLine, Qt::RoundCap);
        p.setPen(track);
        p.drawArc(QRect(6, 6, 158, 158), 0, 360 * 16);

        // value arc — starts at top (90°), sweeps clockwise; Qt angles are
        // counterclockwise in 1/16 deg, so start 90*16 and sweep negative.
        const QColor col = QColor(theme::tempColor(m_t));
        QPen val(col, 6, Qt::SolidLine, Qt::RoundCap);
        p.setPen(val);
        const int sweep = qBound(0, m_t, 100) * 360 * 16 / 100;
        p.drawArc(QRect(6, 6, 158, 158), 90 * 16, -sweep);

        // centered readout (like the mock's .mid .t/.l)
        p.save();
        QFont f = font();
        f.setPixelSize(58);
        f.setWeight(QFont::DemiBold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -1.7);
        p.setFont(f);
        p.setPen(col);
        p.drawText(QRect(0, 40, 170, 70), Qt::AlignHCenter | Qt::AlignVCenter,
                   QString("%1°").arg(m_t));
        p.restore();
        p.save();
        QFont l = font();
        l.setPixelSize(13);
        l.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
        p.setFont(l);
        p.setPen(QColor(139, 135, 163));
        p.drawText(QRect(0, 104, 170, 20), Qt::AlignHCenter, "CPU");
        p.restore();
    }

private:
    int m_t = 0;
};

// ---------------- Popup ----------------
Popup::Popup(QSystemTrayIcon *tray, std::function<void()> openRgb, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_tray(tray), m_openRgb(std::move(openRgb))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(theme::stylesheet());
    setMinimumWidth(340);

    auto *panel = new QFrame(this);
    panel->setObjectName("panel");
    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(22, 10, 22, 22);
    lay->setSpacing(6);

    // titlebar: two honest utility buttons (Option 2) + centered title.
    // No window-manager mimicry — row toggles visibility between panels.
    auto *dots = new QHBoxLayout;
    dots->setContentsMargins(0, 0, 0, 0);
    dots->setSpacing(7);
    auto mkBtn = [this](const char *cls, std::function<void()> act,
                        const QString &label) {
        auto *d = new QPushButton(this);
        d->setObjectName(cls);
        d->setFixedHeight(24);
        d->setCursor(Qt::PointingHandCursor);
        d->setToolTip(label);
        d->setAttribute(Qt::WA_LayoutOnEntireRect);
        d->installEventFilter(this);
        m_dotHints[d] = label;
        connect(d, &QPushButton::clicked, this, [this, act] {
            if (act)                    // INVARIANT: empty fn must never fire
                act();                  // (was a live std::bad_function_call)
        });
        return d;
    };
    // Close — a filled pink circle, no glyph (hover swaps the header text)
    auto *closeBtn = mkBtn("closeBtn", [this] { hide(); }, "Close");
    closeBtn->setText(QString());
    closeBtn->setFixedSize(18, 18);
    dots->addWidget(closeBtn);
    m_rgbBtn = mkBtn("utilBtn",
                     [this] { if (m_openRgb) m_openRgb(); }, "RGB panel");
    m_rgbBtn->setText("RGB");
    dots->addWidget(m_rgbBtn);
    dots->addSpacing(10);
    m_baseTitle = QStringLiteral("Nitro AN515-58");
    auto *ttl = new QLabel(m_baseTitle);
    ttl->setObjectName("ttlc");
    ttl->setAlignment(Qt::AlignCenter);
    dots->addWidget(ttl, 1);
    dots->addSpacing(12);
    m_title = ttl;

    // ring: readout painted inside TempRing
    m_ring = new TempRing;

    // stats row: GPU / CPU fan / GPU fan
    auto *stats = new QHBoxLayout;
    stats->setSpacing(0);
    auto statCol = [](const QString &label, QLabel *&val, const QString &suffix) {
        auto *col = new QVBoxLayout;
        col->setSpacing(1);
        auto *vrow = new QHBoxLayout;
        vrow->setContentsMargins(0, 0, 0, 0);
        vrow->setSpacing(0);
        vrow->addStretch();
        val = new QLabel("—");
        val->setObjectName("statVal");
        vrow->addWidget(val);
        auto *u = new QLabel(suffix);
        u->setObjectName("statUnit");
        u->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        vrow->addWidget(u);
        vrow->addStretch();
        auto *b = new QLabel(label);
        b->setObjectName("statLbl");
        b->setAlignment(Qt::AlignCenter);
        col->addLayout(vrow);
        col->addWidget(b);
        auto *w = new QWidget;
        w->setLayout(col);
        return w;
    };
    stats->addWidget(statCol("GPU", m_statGpu, "°"), 1);
    stats->addWidget(statCol("CPU fan", m_statFanC, " rpm"), 1);
    stats->addWidget(statCol("GPU fan", m_statFanG, " rpm"), 1);

    // segmented modes + desc
    m_seg = new Segmented;
    QStringList labels;
    for (const auto &lv : cfg::LEVELS)
        labels << QString::fromLatin1(lv.title);
    m_seg->setOptions(labels);
    m_seg->setPillColor(QColor(255, 255, 255, 28));
    m_seg->setAccentColor(Qt::white);
    connect(m_seg, &Segmented::selected, this, [this](int idx) {
        // Apply immediately (mock behavior) — turbo-lvl stops the guard for
        // manual levels, so a click also exits auto. No staged Apply button.
        const QString key = QLatin1String(cfg::LEVELS[idx].key);
        m_desc->setText(QString("applying %1…").arg(cfg::LEVELS[idx].title));
        control::setLevelAsync(key, [this, idx](bool ok) {
            if (ok)
                toast(QString("%1 applied").arg(cfg::LEVELS[idx].title));
            else
                toast(QStringLiteral("apply failed — see journalctl -t nitro-priv"));
            refresh();
        });
    });
    m_desc = new QLabel(QString::fromLatin1(cfg::LEVELS[1].sub));
    m_desc->setObjectName("status");

    // fan control rows — CPU and GPU are INDEPENDENT; "All" sets both once
    // (no reverse-sync: moving CPU later never moves GPU/All — the mirrored
    // relay is what dragged every slider with a single-fan change).
    auto fanRow = [this](const char *name, QSlider *&s, QLabel *&val) {
        s = new QSlider(Qt::Horizontal);
        s->setRange(0, 100);
        val = new QLabel("auto");
        val->setObjectName("lab");
        s->setToolTip("Manual duty (this fan only)\n"
                      "0% ≈ 1800 rpm (firmware floor — fans never fully stop)\n"
                      "100% ≈ 7500 rpm. Commits on Apply, or when the popup closes.");
        auto *h = new QHBoxLayout;
        h->setContentsMargins(2, 4, 2, 0);
        auto *b = new QLabel(name);
        b->setObjectName("lab");
        h->addWidget(b);
        h->addWidget(s, 1);
        h->addWidget(val);
        // label mirrors anywhere; NO write here
        connect(s, &QSlider::valueChanged, this,
                [this, val](int v) { val->setText(QString("%1%").arg(v)); });
        auto *w = new QWidget;
        w->setLayout(h);
        return w;
    };
    m_fanAll = nullptr;
    auto *allRow = fanRow("All", m_fanAll, m_fanAllVal);
    auto *cpuRow = fanRow("CPU", m_fanCpu, m_fanCpuVal);
    auto *gpuRow = fanRow("GPU", m_fanGpu, m_fanGpuVal);
    connect(m_fanAll, &QSlider::valueChanged, this,
            [this](int) { /* All mirrors label only; commits via Apply */ });
    // Manual commits happen on the APPLY button (bottom of the fan section):
    // release-time writes raced the EC spin-down and looked like values
    // snapping back; an explicit Apply gives deterministic feedback.

    auto *hline = new QFrame(this);
    hline->setObjectName("hline");
    hline->setFixedHeight(1);
    auto *row1 = new QHBoxLayout;
    row1->setContentsMargins(2, 9, 2, 9);
    auto *r1 = new QLabel("Auto fan control");
    r1->setObjectName("row");
    m_auto = new Toggle;
    m_auto->setToolTip("Auto = firmware curve: profile-steered (Quiet/Balanced buttons also pick the profile)\n"
                       "Manual = your slider duty wins (0% ≈ 1800 rpm, 100% ≈ 7500 rpm).\n"
                       "Thermal guard may force Max at ≥88 °C, recovers ≤80 °C.");
    connect(m_auto, &Toggle::toggled, this, [this](bool on) {
        // OPTIMISTIC UI + intent latch: reflect the user's choice instantly.
        // refresh() may not correct the toggle for 4 s (async nbfc write +
        // settle), else the 1 s poll fights the user mid-apply (the bug).
        m_userIntentMs = QDateTime::currentMSecsSinceEpoch();
        m_intentAuto = on;
        if (on) {
            // no pending-write risk: writes commit only on physical release
            control::setFansAuto([this](bool ok) {
                toast(ok ? QStringLiteral("Auto fan control — firmware curve")
                         : QStringLiteral("auto failed — see journalctl -t nitro-priv"));
                refresh();
            });
            m_pending = QStringLiteral("A");
            m_desc->setText(QStringLiteral("applying auto (thermal guard)…"));
            applySelection();
            return;
        }
        // leaving auto: stage the current SAVED duties in the sliders, but
        // do NOT write — the Apply button is the commit point. Tell the user
        // what is live vs staged, so "Apply?" is never a guess.
        m_userIntentMs = QDateTime::currentMSecsSinceEpoch();
        const auto fans = sensors::readFans();
        const int c = fans.size() >= 1 ? fans[0].cmdDuty : -1;
        const int g2 = fans.size() >= 2 ? fans[1].cmdDuty : -1;
        m_desc->setText(QString("staged: CPU %1% / GPU %2% — Apply to write")
                            .arg(c >= 0 ? c : 0)
                            .arg(g2 >= 0 ? g2 : 0));
    });
    row1->addWidget(r1);
    row1->addStretch();
    row1->addWidget(m_auto);
    auto *row1w = new QWidget;
    row1w->setLayout(row1);

    auto *row2 = new QHBoxLayout;
    row2->setContentsMargins(2, 9, 2, 9);
    auto *r2 = new QLabel("Thermal guard");
    r2->setObjectName("row");
    m_alertRow = new QLabel(QString("alert at %1° · recovery %2°")
                                .arg(cfg::HOT_C).arg(cfg::COOL_C));
    m_alertRow->setObjectName("rowCh");
    row2->addWidget(r2);
    row2->addStretch();
    row2->addWidget(m_alertRow);
    auto *row2w = new QWidget;
    row2w->setLayout(row2);

    lay->addLayout(dots);
    lay->addSpacing(6);
    lay->addWidget(m_ring, 0, Qt::AlignCenter);
    lay->addSpacing(6);
    lay->addLayout(stats);
    lay->addSpacing(14);
    lay->addWidget(m_seg);
    lay->addSpacing(2);
    lay->addWidget(m_desc);
    lay->addSpacing(2);
    lay->addWidget(allRow);
    lay->addWidget(cpuRow);
    lay->addWidget(gpuRow);
    // explicit commit for manual duty (deterministic vs EC spin-down lag)
    auto *fanApply = new QPushButton("Apply fan speeds");
    fanApply->setObjectName("applyBtn");
    fanApply->setCursor(Qt::PointingHandCursor);
    connect(fanApply, &QPushButton::clicked, this, [this] {
        commitFans(m_fanCpu->value(), m_fanGpu->value(), /*notify=*/true);
    });
    lay->addWidget(fanApply);
    lay->addWidget(hline);
    lay->addWidget(row1w);
    lay->addWidget(row2w);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    // refresh/anim timers — only run while visible
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &Popup::refresh);

    m_anim = new QTimer(this);
    m_anim->setInterval(70);
    connect(m_anim, &QTimer::timeout, this, [this] {
        if (!m_fansKnown)
            return;   // keep the dash; nothing to ease
        const int stepC = qBound(-300, m_targetCpuRpm - m_uiCpuRpm, 300);
        const int stepG = qBound(-300, m_targetGpuRpm - m_uiGpuRpm, 300);
        if (stepC) {
            m_uiCpuRpm = qMax(0, m_uiCpuRpm + stepC);
            m_statFanC->setText(QLocale().toString(m_uiCpuRpm));
        }
        if (stepG) {
            m_uiGpuRpm = qMax(0, m_uiGpuRpm + stepG);
            m_statFanG->setText(QLocale().toString(m_uiGpuRpm));
        }
    });

    // drag from any non-interactive area
    for (QWidget *child : findChildren<QWidget *>())
        child->installEventFilter(this);
    installEventFilter(this);
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

void Popup::refresh()
{
    const auto [cpu, gpu] = sensors::readTemps();
    const QString lvl = control::readLevel();
    const auto fans = sensors::readFans();

    m_ring->setTemp(cpu);
    m_statGpu->setText(QString::number(gpu));

    // nbfc `cur` is % of max duty; steps = nominal max RPM. `tgt` is also %.
    // autoCtl = real "Auto Control Enabled" from nbfc — the toggle reflects
    // THIS, not the guard unit (slider writes flip nbfc to manual even while
    // the guard service is running).
    bool nbfcAuto = false;
    int cRpm = 0, gRpm = 0;
    if (fans.size() >= 2) {
        cRpm = nominalMaxRpm(fans[0]);
        gRpm = nominalMaxRpm(fans[1]);
        m_fansKnown = true;
        nbfcAuto = fans[0].autoCtl && fans[1].autoCtl;
        // echo real duty back into the sliders — BUT target duty is
        // transiently 0 during EC transitions and idle manual (Current≠0,
        // Target=0). Echoing that zero stomps the user's setting: the
        // "sliders snap to 0" bug. Echo Strategy:
        //   tgt > 0            → mirror target (authoritative)
        //   tgt == 0, cur > 0  → mirror current (EC still presses air; the
        //                         target-0 is a transition/idle artifact)
        //   both == 0          → mirror 0 (deliberate idle, fans parked)
        // Never echo while the user is dragging, and never inside the
        // 4 s intent window (fresh write may not be reflected yet).
        // Echo the COMMANDED per-fan duty. In MANUAL with a known command:
        // commanded value; in MANUAL with unknown command (helper files not
        // yet written this session, cmdDuty=-9): leave the sliders EXACTLY
        // where the user put them — never reposition from the tach (the
        // screencast bug: sliders drifted toward the easing RPM).
        // In AUTO: sliders hold the user's last committed intent and the
        // label says "auto". The old design echoed the live tach — right
        // after Max→Quiet that showed ~100% on a DISABLED slider, which
        // reads as a bug. Live RPM belongs in the stat boxes only.
        const bool manualC = !fans[0].autoCtl;
        const bool manualG = !fans[1].autoCtl;
        const bool knownC = manualC && fans[0].cmdDuty >= 0;
        const bool knownG = manualG && fans[1].cmdDuty >= 0;
        int showC = knownC ? fans[0].cmdDuty : m_fanCpu->value();
        int showG = knownG ? fans[1].cmdDuty : m_fanGpu->value();
        const qint64 sinceIntent =
            QDateTime::currentMSecsSinceEpoch() - m_userIntentMs;
        const bool inGrace = sinceIntent < 4000;
        m_programmatic = true;   // silent propagation — no commit relay
        if (!inGrace && !m_fanCpu->isSliderDown() &&
            knownC && m_fanCpu->value() != showC)
            m_fanCpu->setValue(showC);
        if (!inGrace && !m_fanGpu->isSliderDown() &&
            knownG && m_fanGpu->value() != showG)
            m_fanGpu->setValue(showG);
        m_fanCpuVal->setText(knownC ? QString("%1%").arg(showC)
                                    : QStringLiteral("auto"));
        m_fanGpuVal->setText(knownG ? QString("%1%").arg(showG)
                                    : QStringLiteral("auto"));
        m_fanAllVal->setText(
            (knownC && knownG && showC == showG) ? QString("%1%").arg(showC)
                                                 : QStringLiteral("—"));
        if (knownC) m_appliedC = showC;   // persisted command = applied truth
        if (knownG) m_appliedG = showG;
        m_programmatic = false;
    } else {
        m_fansKnown = false;   // nbfc missing → dash, not fake 0
    }
    m_targetCpuRpm = cRpm;
    m_targetGpuRpm = gRpm;
    m_statFanC->setText(m_fansKnown ? QLocale().toString(cRpm) : "—");
    m_statFanG->setText(m_fansKnown ? QLocale().toString(gRpm) : "—");

    applyLevel(lvl, m_fansKnown ? nbfcAuto : true);
}

void Popup::applyLevel(const QString &lvl, bool fansAuto)
{
    m_level = lvl;
    // Toggle = real nbfc auto state — but NEVER inside the 4 s intent window
    // after a user click (async write hasn't landed; stale echo would fight
    // the user and re-revert the toggle: the auto-toggle bug).
    const qint64 sinceIntent = QDateTime::currentMSecsSinceEpoch() - m_userIntentMs;
    const bool inGrace = sinceIntent < 4000;
    if (!inGrace && m_auto->isChecked() != fansAuto) {
        QSignalBlocker b(m_auto);
        m_auto->setChecked(fansAuto);
    }
    // Segments stay enabled always — clicking one exits nbfc-auto and applies
    // that level. Pill only moves when a manual level matches.
    // Fan sliders are manual-duty inputs: disabled while nbfc-auto owns the
    // fans (the curve writes duty every second — slider input would fight it).
    const bool slidersEnabled = !m_auto->isChecked();
    for (auto *s : {m_fanAll, m_fanCpu, m_fanGpu})
        if (s->isEnabled() != slidersEnabled)
            s->setEnabled(slidersEnabled);
    if (m_pending.isEmpty()) {
        bool known = false;
        for (int i = 0; i < cfg::N_MANUAL; ++i)
            if (lvl == QLatin1String(cfg::LEVELS[i].key)) {
                if (m_seg->current() != i)
                    m_seg->select(i, false, /*silent=*/true);   // UI sync only!
                m_desc->setText(QString("%1 · Fan: %2")
                                    .arg(QLatin1String(cfg::LEVELS[i].sub),
                                         QLatin1String(cfg::LEVELS[i].fan)));
                known = true;
                break;
            }
        if (!known && !fansAuto)
            m_desc->setText("manual");   // level unreadable but fans pinned
        else if (!known && fansAuto)
            m_desc->setText("Dynamic · adapts to temperature.");
    }
}

void Popup::applySelection()
{
    if (m_pending.isEmpty())
        return;
    const QString applied = m_pending;
    m_pending.clear();
    if (applied == "A")
        m_desc->setText("applying auto (thermal guard)…");
    else
        for (const auto &lv : cfg::LEVELS)
            if (applied == QLatin1String(lv.key)) {
                m_desc->setText(QString("applying %1…").arg(lv.title));
                break;
            }
    control::setLevelAsync(applied, [this, applied](bool ok) {
        if (!ok)
            toast(QStringLiteral("apply failed — see journalctl -t nitro-priv"));
        refresh();
    });
}

void Popup::applyFans()
{
    // INVARIANT: called ONLY from the Apply button (user intent). The old
    // toggle-off call wrote the eased TACH value as duty (helper journal:
    // 'fan cpu 59 / fan gpu 57' pairs every second after each auto-toggle —
    // the ghost pairing). Auto never calls here anymore at all.
    if (m_auto->isChecked())
        return;
    commitFans(m_fanCpu->value(), m_fanGpu->value(), /*notify=*/true);
}

void Popup::commitFans(int c, int g, bool notify)
{
    // Single commit point for both the Apply button and popup-close
    // (apply-before-close so a dragged-but-unapplied slider isn't lost).
    m_userIntentMs = QDateTime::currentMSecsSinceEpoch();
    const auto toastDone = [this, c, g](bool ok) {
        if (ok) {
            m_appliedC = c;
            m_appliedG = g;
            toast(QString("Fans set: CPU %1% · GPU %2%").arg(c).arg(g));
        } else {
            toast(QStringLiteral("fan apply failed — see journalctl -t nitro-priv"));
        }
        refresh();
    };
    // CHAIN: write cpu → write gpu → ONE refresh after both state saves
    // landed. (Firing per-write refreshes let CPU's refresh paint the
    // pre-GPU state file → sliders snapped back to the old values.)
    if (c == g) {
        control::setFanPct(c, -1, toastDone);
    } else {
        const auto afterCpu = [this, g, toastDone](bool okC) {
            if (!okC) { toastDone(false); return; }
            control::setFanPct(g, 1, toastDone);
        };
        control::setFanPct(c, 0, afterCpu);
    }
}

void Popup::toast(const QString &msg)
{
    if (m_tray)
        m_tray->showMessage(QStringLiteral("Nitro Control"), msg,
                            QSystemTrayIcon::Information, 4000);
}

void Popup::place()
{
    if (!m_tray)
        return;
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

void Popup::toggleMax()
{
    if (m_maximized) {
        setGeometry(m_normalGeo);
        m_maximized = false;
    } else {
        m_normalGeo = geometry();
        QScreen *screen = QGuiApplication::screenAt(geometry().center());
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        setGeometry(screen->availableGeometry());
        m_maximized = true;
    }
}

// shared-drag helpers identical with RGB panel (kept intentionally tiny)
bool Popup::eventFilter(QObject *watched, QEvent *event)
{
    // hover hint: swap the header text itself to the dot's purpose
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

void Popup::hideEvent(QHideEvent *ev)
{
    m_timer->stop();
    m_anim->stop();
    // Apply-before-close: a dragged-but-unapplied slider is an intent, not a
    // throwaway — commit it so closing the popup never loses the config.
    // (Auto mode owns the fans: sliders are its live mirror, nothing pending.)
    if (!m_auto->isChecked() && m_fansKnown) {
        const int c = m_fanCpu->value(), g = m_fanGpu->value();
        if (c != m_appliedC || g != m_appliedG)
            commitFans(c, g, /*notify=*/false);
    }
    QWidget::hideEvent(ev);
}

void Popup::keyPressEvent(QKeyEvent *ev)
{
    const QString t = ev->text();
    if (t.size() == 1 && t.at(0).isDigit()) {
        const int idx = t.toInt() - 1;
        if (idx >= 0 && idx < cfg::N_MANUAL) {
            m_auto->setChecked(false);
            m_seg->select(idx);
            return;
        }
    }
    if (t.compare("a", Qt::CaseInsensitive) == 0 || t == "5") {
        m_auto->setChecked(true);
        return;
    }
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
        applySelection();
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
