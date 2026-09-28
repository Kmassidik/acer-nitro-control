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
    void pulseTick();
    void buildMenu();
    void openPopup();
    void openRgb();

    Popup *m_popup = nullptr;
    RgbPanel *m_rgb = nullptr;
    QMenu *m_menu = nullptr;
    QVector<QAction *> m_levelActs;
    QAction *m_info = nullptr;
    QTimer *m_poll = nullptr, *m_pulse = nullptr;
    bool m_pulseOn = false;
};
