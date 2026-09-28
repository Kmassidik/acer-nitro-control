#include "levelbutton.h"

#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>

LevelButton::LevelButton(const QString &key, const QString &title,
                         const QString &sub, QWidget *parent)
    : QFrame(parent), m_key(key)
{
    setObjectName("lvlBtn");
    setCursor(Qt::PointingHandCursor);
    setProperty("active", false);
    setFixedHeight(52);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 8, 6, 7);
    lay->setSpacing(1);
    m_title = new QLabel(title, this);
    m_title->setObjectName("btnTitle");
    m_title->setAlignment(Qt::AlignCenter);
    m_sub = new QLabel(sub, this);
    m_sub->setObjectName("btnSub");
    m_sub->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_title);
    lay->addWidget(m_sub);
}

void LevelButton::setActive(bool on)
{
    if (on == m_active)
        return;
    m_active = on;
    setProperty("active", on);
    style()->unpolish(this);
    style()->polish(this);
}

void LevelButton::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked(m_key);
    QFrame::mousePressEvent(ev);
}
