#pragma once
#include <QHash>
#include <QPoint>
#include <QRect>
#include <QSystemTrayIcon>
#include <QWidget>
#include <functional>

class QLabel;
class QPushButton;
class QTimer;
class LevelButton;
class QMouseEvent;

class Popup : public QWidget
{
    Q_OBJECT
public:
    Popup(QSystemTrayIcon *tray, std::function<void()> openRgb,
          QWidget *parent = nullptr);
    void toggle();
    void refresh();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

private:
    void place();
    void updateBlocks();
    void updateModeUi(const QString &liveLevel);
    void toggleMax();

    QSystemTrayIcon *m_tray;
    std::function<void()> m_openRgb;
    QLabel *m_cpuBlock, *m_gpuBlock;
    QPushButton *m_applyBtn = nullptr;
    QLabel *m_modeStatus = nullptr;
    QHash<QString, LevelButton *> m_btns;
    QTimer *m_timer;
    QTimer *m_anim;

    QString m_level = "?";
    QString m_pendingLevel;
    int m_cpuTempVal = 0, m_gpuTempVal = 0;
    bool m_dragging = false;
    QPoint m_dragPos;
    bool m_maximized = false;
    QRect m_normalGeo;
    int m_targetCpuRpm = 0;
    int m_targetGpuRpm = 0;
    int m_uiCpuRpm = 0;
    int m_uiGpuRpm = 0;
    double m_cpuPhase = 0;
    double m_gpuPhase = 0;
    bool m_barLineRed = false;
};
