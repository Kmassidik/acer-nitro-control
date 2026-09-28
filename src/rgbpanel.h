#pragma once
#include <QColor>
#include <QFrame>
#include <QHash>
#include <QJsonObject>
#include <QWidget>

class Swatch : public QFrame
{
    Q_OBJECT
public:
    explicit Swatch(QWidget *parent = nullptr);
    void setColor(const QColor &c);
    QColor color() const { return m_c; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *ev) override;

private:
    QColor m_c;
};

class QLabel;
class QPushButton;
class QSlider;
class LevelButton;

class RgbPanel : public QWidget
{
    Q_OBJECT
public:
    explicit RgbPanel(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *ev) override;

private:
    void loadUi();
    void pickZone(int idx);
    void pickFx();
    void apply();

    QJsonObject m_state;
    Swatch *m_zones[4];
    Swatch *m_fx;
    QPushButton *m_sync;
    QHash<QString, LevelButton *> m_modes;
    QSlider *m_speed, *m_bright;
    QLabel *m_speedVal, *m_brightVal, *m_status;
};
