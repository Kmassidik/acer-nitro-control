#include "icon.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>

namespace icon {

static constexpr int SIZE = 64;

// Four-blade rotor — the "logo". Blades drawn as curved teardrops around a
// hub; the temp value paints in the rotor's center so the icon stays readable.
static void paintRotor(QPainter &p, const QRectF &r, const QColor &col)
{
    p.save();
    p.translate(r.center());
    const double R = r.width() / 2.0;

    // blades: 4 teardrops rotated 90° apart; tip touches rim, wide end at hub
    QPainterPath blade;
    blade.moveTo(0, -R * 0.86);                       // tip (near rim)
    blade.cubicTo(R * 0.34, -R * 0.62,                // outer curve
                  R * 0.30, -R * 0.20, 0, -R * 0.16); // sweep into hub
    blade.cubicTo(-R * 0.16, -R * 0.26,               // inner curve back
                  -R * 0.20, -R * 0.66, 0, -R * 0.86);

    p.setPen(Qt::NoPen);
    p.setBrush(col);
    for (int i = 0; i < 4; ++i) {
        p.save();
        p.rotate(i * 90.0);
        p.drawPath(blade);
        p.restore();
    }

    // hub with center dot (bearing)
    p.setBrush(col);
    p.drawEllipse(QPointF(0, 0), R * 0.155, R * 0.155);
    p.setBrush(QColor("#14131c"));
    p.drawEllipse(QPointF(0, 0), R * 0.06, R * 0.06);
    p.restore();
}

QIcon makeIcon(int t, const QString &lvl, bool pulse)
{
    QPixmap pm(SIZE * 2, SIZE * 2);
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor col = pulse ? QColor("#f87171")
                             : QColor(t < 72 ? "#a78bfa" : t < 85 ? "#fbbf24"
                                                                  : "#f87171");

    // faint track ring behind the rotor (like the mock's stroke #ffffff10)
    QPen track(QColor(255, 255, 255, 16), 3.0);
    p.setPen(track);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(9, 9, 46, 46));

    // rotor saturated with the temp color; blades lose their fill while
    // pulsing (blink effect) but the outline stays visible
    if (pulse) {
        QPainterPath blades;
        blades.moveTo(32, 6);
        blades.cubicTo(54, 17, 54, 29, 32, 26);
        blades.cubicTo(22, 24, 21, 13, 32, 6);
        p.setPen(QPen(col, 1.6));
        p.setBrush(Qt::NoBrush);
        for (int i = 0; i < 4; ++i) {
            p.save();
            p.translate(32, 32);
            p.rotate(i * 90.0);
            p.translate(-32, -32);
            p.drawPath(blades);
            p.restore();
        }
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawEllipse(QPointF(32, 32), 5.5, 5.5);
    } else {
        paintRotor(p, QRectF(9, 9, 46, 46), col);
    }

    // temp value in the rotor's hub area
    p.setPen(QColor("#e8e6f5"));
    QFont f("JetBrains Mono", 11, QFont::Bold);
    p.setFont(f);
    p.drawText(QRectF(0, 0, SIZE, SIZE),
               Qt::AlignHCenter | Qt::AlignVCenter, QString::number(t));

    // level badge under the rotor
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
