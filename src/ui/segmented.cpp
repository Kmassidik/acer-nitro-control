#include "ui/segmented.h"

#include <QEasingCurve>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>

// ---------------- Segmented ----------------
Segmented::Segmented(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    m_frame = new QFrame(this);
    m_frame->setObjectName("seg");
    outer->addWidget(m_frame);

    m_lay = new QHBoxLayout(m_frame);
    m_lay->setContentsMargins(3, 3, 3, 3);
    m_lay->setSpacing(0);

    m_pill = new QWidget(m_frame);
    m_pill->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_pillColor = QColor(255, 255, 255, 28);
    setPillColor(m_pillColor);
}

void Segmented::setOptions(const QStringList &labels)
{
    for (auto *b : m_btns) {
        m_lay->removeWidget(b);
        b->deleteLater();
    }
    m_btns.clear();
    for (int i = 0; i < labels.size(); ++i) {
        auto *b = new QToolButton(m_frame);
        b->setObjectName("segBtn");
        b->setText(labels[i]);
        b->setCheckable(true);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        const int idx = i;
        connect(b, &QToolButton::clicked, this, [this, idx] { select(idx); });
        m_lay->addWidget(b, 1);
        m_btns.append(b);
    }
    applyAccent();
    m_idx = 0;
    for (auto *b : m_btns)
        b->setChecked(false);
    // pill needs child geometry — place after layout settles
    QMetaObject::invokeMethod(this, [this] { placePill(m_idx, false); },
                              Qt::QueuedConnection);
}

void Segmented::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
    if (auto *b = m_btns.value(m_idx); b && b->width() > 0)
        placePill(m_idx, false);
}

void Segmented::placePill(int idx, bool animated)
{
    if (idx < 0 || idx >= m_btns.size())
        return;
    auto *b = m_btns[idx];
    if (b->width() <= 0)
        return;   // not laid out yet; resizeEvent will re-place
    const QRect target(b->geometry());
    if (!animated) {
        m_pill->setGeometry(target);
        m_pill->raise();
        return;
    }
    auto *anim = new QPropertyAnimation(m_pill, "geometry", this);
    anim->setDuration(280);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(m_pill->geometry());
    anim->setEndValue(target);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Segmented::select(int idx, bool animated)
{
    if (idx < 0 || idx >= m_btns.size() || idx == m_idx)
        return;
    m_idx = idx;
    for (int i = 0; i < m_btns.size(); ++i)
        m_btns[i]->setChecked(i == idx);
    applyAccent();
    placePill(idx, animated);
    emit selected(idx);
}

void Segmented::setEnabled(bool on)
{
    QWidget::setEnabled(on);
    for (auto *b : m_btns)
        b->setEnabled(on);
}

void Segmented::setPillColor(const QColor &c)
{
    m_pillColor = c;
    m_pill->setStyleSheet(QString(
        "background: %1; border-radius: 9px; border: none;")
                              .arg(c.name(QColor::HexArgb)));
}

void Segmented::setAccentColor(const QColor &c)
{
    m_accent = c;
    applyAccent();
}

void Segmented::applyAccent()
{
    for (auto *b : m_btns)
        b->setStyleSheet(b->isChecked()
                             ? QString("QToolButton#segBtn { color: %1; }")
                                   .arg(m_accent.name())
                             : QString());
}

// ---------------- Toggle ----------------
Toggle::Toggle(QWidget *parent) : QToolButton(parent)
{
    setCheckable(true);
    setFixedSize(36, 22);
    setCursor(Qt::PointingHandCursor);
}

void Toggle::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // track
    p.setPen(Qt::NoPen);
    p.setBrush(isChecked() ? QColor("#34d399") : QColor(255, 255, 255, 34));
    p.drawRoundedRect(rect(), 11, 11);
    // knob
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    const int d = 18;
    const int x = isChecked() ? width() - d - 2 : 2;
    p.drawEllipse(x, 2, d, d);
}
