#include "levelbutton.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyle>

LevelButton::LevelButton(const QString &key, const QString &title,
                         const QString &sub, QWidget *parent)
    : QFrame(parent), m_key(key), m_titleText(title), m_subText(sub)
{
    setObjectName("lvlBtn");
    setCursor(Qt::PointingHandCursor);
    setProperty("active", false);
    setFixedHeight(26);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(8, 2, 8, 2);
    lay->setSpacing(6);
    m_title = new QLabel(QStringLiteral("  ") + title, this);
    m_title->setObjectName("btnTitle");
    m_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_sub = new QLabel(sub, this);
    m_sub->setObjectName("btnSub");
    m_sub->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(m_title);
    lay->addStretch();
    lay->addWidget(m_sub);
}

void LevelButton::setActive(bool on)
{
    if (on == m_active)
        return;
    m_active = on;
    setProperty("active", on);
    m_title->setText((on ? QStringLiteral("▸ ") : QStringLiteral("  ")) + m_titleText);
    QFont f = m_title->font();
    f.setWeight(on ? QFont::Bold : QFont::Normal);
    m_title->setFont(f);
    m_title->setStyleSheet(on
        ? QStringLiteral("color:#cdd6f4;")
        : QStringLiteral("color:#9399b2;"));
    m_sub->setStyleSheet(on
        ? QStringLiteral("color:#6c7086;")
        : QStringLiteral("color:#6c7086;"));
    style()->unpolish(this);
    style()->polish(this);
}

void LevelButton::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked(m_key);
    QFrame::mousePressEvent(ev);
}
