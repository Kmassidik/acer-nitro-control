#pragma once
#include <QSystemTrayIcon>

class QMenu;
class QAction;
class QTimer;
class Popup;
class RgbPanel;

class Tray : public QSystemTrayIcon
{
    Q_OBJECT
public:
    explicit Tray(QObject *parent = nullptr);

private:
    void poll();
    void pulseTick();         // icon animation only — no subprocess reads
    void buildMenu();
    void openPopup();
    void openRgb();
    void nudgeRgb(int delta);   // Fn+F9/F10 global shortcut

    Popup *m_popup = nullptr;
    RgbPanel *m_rgb = nullptr;
    QMenu *m_menu = nullptr;
    QVector<QAction *> m_levelActs;   // 5 acts: 4 manual + auto
    QAction *m_info = nullptr;
    QTimer *m_poll = nullptr, *m_pulse = nullptr;
    int m_lastCpu = 45;      // cached from poll() — pulseTick reuses it
    QString m_lastLvl = "?";
    bool m_pulseOn = false;
};
