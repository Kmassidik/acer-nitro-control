#pragma once
#include <QFrame>
#include <QLabel>

class LevelButton : public QFrame
{
    Q_OBJECT
public:
    LevelButton(const QString &key, const QString &title,
                const QString &sub, QWidget *parent = nullptr);
    void setActive(bool on);
    QString key() const { return m_key; }

signals:
    void clicked(const QString &key);

protected:
    void mousePressEvent(QMouseEvent *ev) override;

private:
    QString m_key;
    QLabel *m_title, *m_sub;
    bool m_active = false;
    QString m_titleText, m_subText;
};
