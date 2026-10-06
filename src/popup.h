#pragma once
#include <QHash>
#include <QSystemTrayIcon>
#include <QWidget>
#include <functional>

class QLabel;
class QPushButton;
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
    void updateBlocks();

    QSystemTrayIcon *m_tray;
    std::function<void()> m_openRgb;
    QLabel *m_cpuBlock, *m_gpuBlock, *m_log;
    QHash<QString, LevelButton *> m_btns;
    QTimer *m_timer;
    QTimer *m_anim, *m_cursorBlink;
    QLabel *m_cursor;

    QString m_level = "?";
    int m_cpuTempVal = 0, m_gpuTempVal = 0;
    int m_targetCpuRpm = 0;
    int m_targetGpuRpm = 0;
    int m_uiCpuRpm = 0;
    int m_uiGpuRpm = 0;
    double m_cpuPhase = 0;
    double m_gpuPhase = 0;
    bool m_barLineRed = false;
};
