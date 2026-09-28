#include "tray.h"
#include "config.h"
#include "control.h"
#include "icon.h"
#include "popup.h"
#include "rgbpanel.h"
#include "sensors.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QMenu>
#include <QTimer>

Tray::Tray(QObject *parent) : QSystemTrayIcon(parent)
{
    setIcon(icon::makeIcon(45, "?", false));
    setToolTip("Nitro Control");
    buildMenu();

    m_poll = new QTimer(this);
    m_poll->setInterval(cfg::POLL_MS);
    connect(m_poll, &QTimer::timeout, this, &Tray::poll);
    m_poll->start();

    m_pulse = new QTimer(this);
    m_pulse->setInterval(cfg::PULSE_MS);
    connect(m_pulse, &QTimer::timeout, this, &Tray::pulseTick);

    connect(this, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) {
                if (r == QSystemTrayIcon::Trigger)
                    openPopup();
            });
    poll();
}

void Tray::buildMenu()
{
    m_menu = new QMenu;
    auto *group = new QActionGroup(m_menu);
    group->setExclusive(true);
    for (const auto &lv : cfg::LEVELS) {
        auto *act = group->addAction(QString::fromLatin1(lv.menu));
        act->setCheckable(true);
        act->setData(QString::fromLatin1(lv.key));
        connect(act, &QAction::triggered, this, [this, act] {
            control::setLevel(act->data().toString());
            poll();
        });
        m_levelActs.append(act);
    }
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
        m_rgb = new RgbPanel;
        const QRect g = geometry();
        m_rgb->move(g.center().x() - m_rgb->width() / 2, g.bottom() + 8);
    }
    m_rgb->show();
    m_rgb->raise();
}

void Tray::poll()
{
    const auto temps = sensors::readTemps();
    const int cpu = temps.first, gpu = temps.second;
    const QString lvl = control::readLevel();

    int lvlIdx = -1;
    for (int i = 0; i < 6; ++i)
        if (lvl == QLatin1String(cfg::LEVELS[i].key))
            lvlIdx = i;
    for (int i = 0; i < m_levelActs.size(); ++i)
        if (m_levelActs[i]->isChecked() != (i == lvlIdx))
            m_levelActs[i]->setChecked(i == lvlIdx);
    m_info->setText(QString("CPU %1°C · GPU %2°C").arg(cpu).arg(gpu));

    const int mx = qMax(cpu, gpu);
    if (mx >= cfg::HOT_C && !m_pulse->isActive())
        m_pulse->start();
    if (mx <= cfg::COOL_C && m_pulse->isActive()) {
        m_pulse->stop();
        m_pulseOn = false;
    }
    setIcon(icon::makeIcon(cpu, lvl == QLatin1String("?") ? "?" : lvl, m_pulseOn));
    setToolTip(QString("Nitro Control — CPU %1°C · GPU %2°C · %3")
                   .arg(cpu).arg(gpu)
                   .arg(lvl == QLatin1String("A") ? "auto" : lvl));
}

void Tray::pulseTick()
{
    m_pulseOn = !m_pulseOn;
    const auto temps = sensors::readTemps();
    const QString lvl = control::readLevel();
    setIcon(icon::makeIcon(temps.first,
                           lvl == QLatin1String("?") ? "?" : lvl, m_pulseOn));
}
