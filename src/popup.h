#pragma once
#include <QPoint>
#include <QRect>
#include <QSystemTrayIcon>
#include <QWidget>
#include <functional>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QAbstractButton;
class QAbstractSlider;
class QMouseEvent;
class Segmented;
class Toggle;
class TempRing;

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
    void applyLevel(const QString &lvl, bool fansAuto);
    void applySelection();
    void applyFans();
    void scheduleFanWrite();
    void place();
    void toggleMax();

    QSystemTrayIcon *m_tray = nullptr;
    std::function<void()> m_openRgb;

    TempRing *m_ring = nullptr;
    QLabel *m_statGpu = nullptr;
    QLabel *m_statFanC = nullptr;
    QLabel *m_statFanG = nullptr;
    Segmented *m_seg = nullptr;
    QLabel *m_desc = nullptr;
    Toggle *m_auto = nullptr;
    QLabel *m_alertRow = nullptr;
    QSlider *m_fanAll = nullptr, *m_fanCpu = nullptr, *m_fanGpu = nullptr;
    QLabel *m_fanAllVal = nullptr, *m_fanCpuVal = nullptr, *m_fanGpuVal = nullptr;
    bool m_programmatic = false;   // suppress slider echoes while syncing UI
    QTimer *m_fanDebounce = nullptr;   // coalesce slider movement before nbfc write
    qint64 m_lastFanWriteMs = 0;   // debounce against 1s refresh re-apply
    qint64 m_userIntentMs = 0;     // last toggle/segment click — echo suppress window
    bool m_intentAuto = false;     // what the user last asked auto to be

    QString m_level = "?";
    QString m_pending;
    int m_targetCpuRpm = 0, m_targetGpuRpm = 0;   // real, from nbfc
    int m_uiCpuRpm = 0, m_uiGpuRpm = 0;           // smoothed for display
    bool m_fansKnown = false;                     // false = nbfc unreachable
    bool m_dragging = false;
    QPoint m_dragPos;
    bool m_maximized = false;
    QRect m_normalGeo;
    QTimer *m_timer = nullptr;   // refresh (only while visible)
    QTimer *m_anim = nullptr;    // rpm easing (only while visible)
};
