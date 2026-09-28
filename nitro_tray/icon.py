"""Tray gauge icon: temperature ring + level badge (HiDPI 2x)."""
from PySide6.QtCore import QRectF, Qt
from PySide6.QtGui import QColor, QFont, QIcon, QPainter, QPen, QPixmap

from nitro_tray.theme import temp_color

_SIZE = 64


def make_icon(t, lvl="?", pulse=False):
    """64px logical gauge: arc = temp/100, center = temp, badge = level ('!' when hot)."""
    pm = QPixmap(_SIZE * 2, _SIZE * 2)
    pm.setDevicePixelRatio(2.0)
    pm.fill(QColor(0, 0, 0, 0))
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    col = QColor("#ef4444") if pulse else temp_color(t)
    rect = QRectF(7, 7, 50, 50)

    p.setPen(QPen(QColor("#444444"), 5.5, Qt.SolidLine, Qt.RoundCap))
    p.setBrush(Qt.NoBrush)
    p.drawArc(rect, 0, 360 * 16)

    sweep = -max(0, min(100, t)) / 100 * 360 * 16
    p.setPen(QPen(col, 5.5, Qt.SolidLine, Qt.RoundCap))
    p.drawArc(rect, 90 * 16, int(sweep))

    p.setPen(QColor("#f2e8d5"))
    p.setFont(QFont("JetBrains Mono", 16, QFont.Bold))
    p.drawText(QRectF(0, 0, _SIZE, _SIZE), int(Qt.AlignHCenter | Qt.AlignVCenter), str(t))

    badge = "!" if pulse else lvl
    p.setBrush(QColor("#141f33"))
    p.setPen(QPen(col, 1.2))
    p.drawRoundedRect(QRectF(22, 50, 20, 13), 5, 5)
    p.setPen(col)
    p.setFont(QFont("JetBrains Mono", 7, QFont.Bold))
    p.drawText(QRectF(22, 50, 20, 13), int(Qt.AlignHCenter | Qt.AlignVCenter), badge)
    p.end()
    return QIcon(pm)
