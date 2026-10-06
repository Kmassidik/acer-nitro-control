#pragma once
#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QPoint>
#include <QRect>
#include <QWidget>
#include <functional>

class QLabel;
class QSlider;
class QPushButton;
class Segmented;
class Toggle;
class QHideEvent;

class Keycap : public QWidget
{
    Q_OBJECT
public:
    Keycap(int row, int col, QWidget *parent = nullptr);
    int row() const { return m_row; }
    int col() const { return m_col; }
    int zone() const { return m_zone; }
    void setZone(int z) { m_zone = z; }
    void setRgb(const QColor &c, double glow);

signals:
    void picked(int row, int col);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *ev) override;

private:
    int m_row, m_col, m_zone = 0;
    QColor m_c{27, 26, 38};
    double m_glow = 0;
};

class RgbPanel : public QWidget
{
    Q_OBJECT
public:
    explicit RgbPanel(QWidget *parent = nullptr, std::function<void()> openPopup = {});

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

private:
    void loadUi();
    void paintKeyboard();
    void selectZone(int idx);
    void pickColorForCurrentZone();   // palette chip or dialog fallback
    void apply();
    void syncUiFromState();

    QJsonObject m_state;
    bool m_dragging = false;
    QPoint m_dragPos;
    bool m_maximized = false;
    QRect m_normalGeo;

    QWidget *m_kb = nullptr;               // 15x5 keycap grid container
    QVector<QVector<Keycap *>> m_keys;     // [row][col]
    Segmented *m_fx = nullptr;
    QLabel *m_zoneLbl = nullptr;
    QVector<QPushButton *> m_zones;        // 4 zone chips
    QWidget *m_pal = nullptr;              // palette chip row (built in ctor)
    QVector<QPushButton *> m_palBtns;
    QSlider *m_bright = nullptr, *m_speed = nullptr;
    QLabel *m_brightVal = nullptr, *m_speedVal = nullptr;
    Toggle *m_link = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_title = nullptr;                       // header text swaps on dot hover
    QString m_baseTitle;
    QHash<QPushButton *, QString> m_dotHints;
    std::function<void()> m_openPopup;
    QTimer *m_paintTimer = nullptr;        // live effect animation (only while visible)
    double m_t = 0;
    int m_selZone = 0;
};
