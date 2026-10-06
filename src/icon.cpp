#include "icon.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>

namespace icon {

static constexpr int SIZE = 64;

QIcon makeIcon(int t, const QString &lvl, bool pulse)
{
    QPixmap pm(SIZE * 2, SIZE * 2);
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QColor col = pulse ? QColor("#f87171")
                       : QColor(t < 72 ? "#a78bfa" : t < 85 ? "#fbbf24" : "#f87171");
    QRectF rect(7, 7, 50, 50);

    QPen bg(QColor(255, 255, 255, 38), 5.5, Qt::SolidLine, Qt::RoundCap);
    p.setPen(bg);
    p.setBrush(Qt::NoBrush);
    p.drawArc(rect, 0, 360 * 16);

    const double sweep = -qBound(0, t, 100) / 100.0 * 360.0 * 16.0;
    QPen fg(col, 5.5, Qt::SolidLine, Qt::RoundCap);
    p.setPen(fg);
    p.drawArc(rect, 90 * 16, int(sweep));

    p.setPen(QColor("#e8e6f5"));
    QFont f("JetBrains Mono", 16, QFont::Bold);
    p.setFont(f);
    p.drawText(QRectF(0, 0, SIZE, SIZE),
               Qt::AlignHCenter | Qt::AlignVCenter, QString::number(t));

    const QString badge = pulse ? QStringLiteral("!") : lvl;
    p.setBrush(QColor("#14131c"));
    p.setPen(QPen(col, 1.2));
    p.drawRoundedRect(QRectF(22, 50, 20, 13), 5, 5);
    p.setPen(col);
    p.setFont(QFont("JetBrains Mono", 7, QFont::Bold));
    p.drawText(QRectF(22, 50, 20, 13),
               Qt::AlignHCenter | Qt::AlignVCenter, badge);
    p.end();
    return QIcon(pm);
}

} // namespace icon
