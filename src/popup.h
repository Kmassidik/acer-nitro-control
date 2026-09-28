#pragma once
#include <QHash>
#include <QSystemTrayIcon>
#include <QWidget>
#include <functional>

class QLabel;
class QProgressBar;
class QTimer;
class LevelButton;

class Popup : public QWidget
{
    Q_OBJECT
public:
    Popup(QSystemTrayIcon *tray, std::function<void()> openRgb,
          QWidget *parent = nullptr);
    void toggle();
    void refresh();

protected:
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void place();

    QSystemTrayIcon *m_tray;
    std::function<void()> m_openRgb;
    QLabel *m_badge, *m_cpuFan, *m_gpuFan, *m_cpuTemp, *m_gpuTemp, *m_load;
    QProgressBar *m_cpuBar, *m_gpuBar;
    QHash<QString, LevelButton *> m_btns;
    QTimer *m_timer;
};
