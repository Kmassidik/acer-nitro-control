#include "ui/tray.h"
#include "core/config.h"
#include "core/control.h"
#include "ui/globalkeys.h"
#include "ui/icon.h"
#include "ui/popup.h"
#include "ui/rgbpanel.h"
#include "core/sensors.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QJsonObject>
#include <QMenu>
#include <QTimer>

Tray::Tray(QObject *parent) : QSystemTrayIcon(parent)
{
    setIcon(icon::makeIcon(45, "?", false));
    setToolTip("Nitro Control");
    buildMenu();

    // Fn+F9 / Fn+F10 → keyboard backlight brightness ( Plasma global
    // shortcuts; the hwdb maps the scancodes to kbdillumdown/up).
    auto *keys = new qkeys::GlobalKeyListener(this);
    if (keys->registerKeys()) {
        connect(keys, &qkeys::GlobalKeyListener::brightUp, this,
                [this] { nudgeRgb(+10); });
        connect(keys, &qkeys::GlobalKeyListener::brightDown, this,
                [this] { nudgeRgb(-10); });
    }

    m_poll = new QTimer(this);
    m_poll->setInterval(cfg::POLL_MS);
    connect(m_poll, &QTimer::timeout, this, &Tray::poll);

    m_pulse = new QTimer(this);
    m_pulse->setInterval(cfg::PULSE_MS);
    connect(m_pulse, &QTimer::timeout, this, &Tray::pulseTick);

    connect(this, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) {
                if (r == QSystemTrayIcon::Trigger)
                    openPopup();
            });
    poll();
    m_poll->start();
    show();   // registers the StatusNotifierItem with the watcher — without
              // this Qt never exports the icon and nothing shows in the tray
}

void Tray::buildMenu()
{
    m_menu = new QMenu;
    auto *group = new QActionGroup(m_menu);
    group->setExclusive(true);
    for (const auto &lv : cfg::LEVELS) {
        auto *act = group->addAction(
            QString("lvl%1  %2 (%3)").arg(lv.key, lv.title, lv.sub));
        act->setCheckable(true);
        act->setData(QString::fromLatin1(lv.key));
        m_levelActs.append(act);
    }
    auto *autoAct = group->addAction("auto  (thermal guard)");
    autoAct->setCheckable(true);
    autoAct->setData("A");
    m_levelActs.append(autoAct);
    for (auto *act : m_levelActs)
        connect(act, &QAction::triggered, this, [this, act] {
            control::setLevelAsync(act->data().toString(),
                                   [this] { QTimer::singleShot(300, this, &Tray::poll); });
        });
    m_menu->addSeparator();
    m_info = m_menu->addAction("—");
    m_info->setEnabled(false);
    m_menu->addSeparator();
    m_menu->addAction("Control panel", this, &Tray::openPopup);
    m_menu->addAction("Keyboard RGB", this, &Tray::openRgb);
    m_menu->addSeparator();
    m_menu->addAction("Quit", qApp, &QCoreApplication::quit);
    setContextMenu(m_menu);
}

void Tray::openPopup()
{
    if (!m_popup)
        m_popup = new Popup(this, [this] { openRgb(); });
    m_popup->toggle();
}

void Tray::openRgb()
{
    if (!m_rgb) {
        m_rgb = new RgbPanel(nullptr, [this] {
            if (!m_popup)
                m_popup = new Popup(this, [this] { openRgb(); });
            m_popup->show();
            m_popup->raise();
            m_rgb->hide();
        });
        const QRect g = geometry();
        m_rgb->move(g.center().x() - m_rgb->width() / 2, g.bottom() + 8);
    }
    m_rgb->show();
    m_rgb->raise();
}

void Tray::nudgeRgb(int delta)
{
    // Fn+F9/F10 handler: read saved state, step brightness, apply + persist.
    QJsonObject st = control::loadRgb();
    const int b = qBound(0, st["brightness"].toInt(40) + delta, 100);
    if (b == st["brightness"].toInt(-1))
        return;   // saturation — no-op, no save spam
    st["brightness"] = b;
    QString err;
    const bool ok = control::applyRgb(st, &err, true);
    setToolTip(QString("Nitro Control — keyboard brightness %1% (%2)")
                   .arg(b).arg(ok ? "✓" : err));
}

void Tray::poll()
{
    const auto [cpu, gpu] = sensors::readTemps();
    const QString lvl = control::readLevel();
    m_lastCpu = cpu;
    m_lastLvl = lvl;

    int lvlIdx = -1;
    const QString key = (lvl == "5") ? "4" : lvl;   // legacy 5-level key map
    for (int i = 0; i < m_levelActs.size(); ++i)
        if (key == m_levelActs[i]->data().toString())
            lvlIdx = i;
    for (int i = 0; i < m_levelActs.size(); ++i)
        if (m_levelActs[i]->isChecked() != (i == lvlIdx))
            m_levelActs[i]->setChecked(i == lvlIdx);
    m_info->setText(QString("CPU %1°C · GPU %2°C").arg(cpu).arg(gpu));

    const int mx = qMax(cpu, gpu);
    if (mx >= cfg::HOT_C && !m_pulse->isActive()) {
        m_pulse->start();
    } else if (mx <= cfg::COOL_C && m_pulse->isActive()) {
        m_pulse->stop();
        m_pulseOn = false;
        setIcon(icon::makeIcon(cpu, key, false));
        setToolTip(QString("Nitro Control — CPU %1°C · GPU %2°C · %3")
                       .arg(cpu).arg(gpu)
                       .arg(lvl == "A" ? "auto" : lvl));
        return;
    }
    setIcon(icon::makeIcon(cpu, key == "?" ? "?" : key, m_pulseOn));
    setToolTip(QString("Nitro Control — CPU %1°C · GPU %2°C · %3")
                   .arg(cpu).arg(gpu)
                   .arg(lvl == "A" ? "auto" : lvl));
}

void Tray::pulseTick()
{
    m_pulseOn = !m_pulseOn;
    // reuse cached temp/level from poll() — no subprocess reads per pulse
    setIcon(icon::makeIcon(m_lastCpu,
                           m_lastLvl == "5" ? "4"
                                              : (m_lastLvl == "A" ? "A" : m_lastLvl),
                           m_pulseOn));
}
