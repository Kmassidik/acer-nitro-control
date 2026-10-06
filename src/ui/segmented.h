#pragma once
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QToolButton>
#include <QWidget>

// Sliding-pill segmented control (Glass Ghost .seg/.pill port).
class Segmented : public QWidget
{
    Q_OBJECT
public:
    explicit Segmented(QWidget *parent = nullptr);
    void setOptions(const QStringList &labels);
    void select(int idx, bool animated = true);
    int current() const { return m_idx; }
    void setEnabled(bool on);          // dim + ignore input (mock .seg.off)
    void setPillColor(const QColor &c);
    void setAccentColor(const QColor &c);   // checked text color

signals:
    void selected(int idx);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override { return QWidget::eventFilter(watched, event); }
    void resizeEvent(QResizeEvent *ev) override;

private:
    void placePill(int idx, bool animated);
    void applyAccent();
    QFrame *m_frame = nullptr;
    QHBoxLayout *m_lay = nullptr;
    QWidget *m_pill = nullptr;
    QVector<QToolButton *> m_btns;
    int m_idx = 0;
    QColor m_pillColor;
    QColor m_accent = Qt::white;
};

// iOS-style switch: track + knob painted in paintEvent (checked = on).
class Toggle : public QToolButton
{
    Q_OBJECT
public:
    explicit Toggle(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
};
