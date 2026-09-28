"""Fedora-glass palette + shared color rules + Qt stylesheet."""
from PySide6.QtGui import QColor

from nitro_tray.config import FONT_BODY, FONT_MONO

# palette tokens (match the HTML mockup)
ACCENT = "#d9a862"
BRIGHT = "#f2e8d5"
FG = "#d8cbb4"
MUTED = "#5e6e87"


def temp_color(t):
    """Gauge/bar color by temperature tier (<60 gold, <75 light, <85 orange, else red)."""
    if t < 60:
        return QColor(ACCENT)
    if t < 75:
        return QColor("#e0b579")
    if t < 85:
        return QColor("#f97316")
    return QColor("#ef4444")


def bar_tier(t):
    """Same tiers as QSS dynamic-property selector (QProgressBar[tier=N])."""
    if t < 60:
        return 0
    if t < 75:
        return 1
    if t < 85:
        return 2
    return 3


QSS = """
QWidget { background: transparent; color: %(fg)s; font-family: "%(body)s"; font-size: 12px; }
QFrame#panel {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #17243a, stop:1 #0c1626);
  border: 1px solid rgba(216,203,180,0.18);
  border-radius: 20px;
}
QLabel#title { color: %(bright)s; font-size: 13px; font-weight: bold; background: transparent; }
QLabel#dot { color: %(accent)s; font-size: 12px; background: transparent; }
QLabel#badge {
  background: rgba(217,168,98,0.14); color: %(accent)s;
  border: 1px solid rgba(217,168,98,0.45); border-radius: 6px;
  padding: 1px 7px; font-family: "%(mono)s"; font-size: 10px; font-weight: bold;
}
QLabel#sect { color: %(fg)s; font-size: 11px; font-weight: 600; background: transparent; }
QLabel#fan { color: %(muted)s; font-size: 10px; font-family: "%(mono)s"; background: transparent; }
QLabel#temp { color: %(bright)s; font-size: 13px; font-weight: bold;
              font-family: "%(mono)s"; background: transparent; }
QProgressBar {
  background: #0a1220; border: 1px solid rgba(94,110,135,0.5);
  border-radius: 8px; height: 16px; text-align: right; padding-right: 7px;
  color: %(bright)s; font-family: "%(mono)s"; font-size: 9px;
}
QProgressBar::chunk { border-radius: 7px; background: %(accent)s; }
QProgressBar[tier="1"]::chunk { background: #e0b579; }
QProgressBar[tier="2"]::chunk { background: #f97316; }
QProgressBar[tier="3"]::chunk { background: #ef4444; }
QFrame#loadcard {
  background: rgba(38,54,78,0.55); border: 1px solid rgba(94,110,135,0.35);
  border-radius: 10px;
}
QLabel#load { color: %(fg)s; font-family: "%(mono)s"; font-size: 10px; background: transparent; }
QLabel#chip {
  background: rgba(217,168,98,0.12); color: %(accent)s;
  border: 1px solid rgba(217,168,98,0.35); border-radius: 6px;
  padding: 1px 6px; font-family: "%(mono)s"; font-size: 10px; font-weight: bold;
}
QFrame#lvlBtn {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(38,54,78,0.75), stop:1 rgba(23,36,58,0.75));
  border: 1px solid rgba(94,110,135,0.5); border-radius: 12px;
}
QFrame#lvlBtn:hover {
  border-color: rgba(217,168,98,0.65);
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(38,54,78,0.95), stop:1 rgba(23,36,58,0.9));
}
QFrame#lvlBtn[active="true"] {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %(accent)s, stop:1 #b8863f);
  border: 1px solid %(bright)s;
}
QLabel#btnTitle { color: %(fg)s; font-size: 11px; font-weight: bold;
                  background: transparent; border: none; }
QLabel#btnSub { color: %(muted)s; font-size: 9px; font-family: "%(mono)s";
                background: transparent; border: none; }
QFrame#lvlBtn[active="true"] QLabel#btnTitle { color: #0c1626; }
QFrame#lvlBtn[active="true"] QLabel#btnSub { color: #4a3c22; }
QLabel#foot { color: %(muted)s; font-size: 10px; font-family: "%(mono)s"; background: transparent; }
QLabel#footG { color: %(accent)s; font-size: 10px; font-family: "%(mono)s"; background: transparent; }
QPushButton#x { color: %(muted)s; border: none; background: transparent; font-size: 15px; padding: 0 4px; }
QPushButton#x:hover { color: %(bright)s; }
QPushButton#rgbBtn {
  background: rgba(38,54,78,0.55); border: 1px solid rgba(94,110,135,0.45);
  border-radius: 10px; color: %(fg)s; font-size: 11px; font-weight: 600; padding: 7px;
}
QPushButton#rgbBtn:hover { border-color: rgba(217,168,98,0.6); color: %(bright)s; }
QFrame#swatch {
  border-radius: 10px; border: 1px solid rgba(216,203,180,0.35); min-width: 44px;
  min-height: 26px; max-height: 26px;
}
QFrame#swatch:hover { border: 1px solid %(accent)s; }
QPushButton#chipBtn {
  background: rgba(38,54,78,0.6); border: 1px solid rgba(94,110,135,0.45);
  border-radius: 8px; color: %(fg)s; font-size: 10px; padding: 4px 9px;
  font-family: "%(mono)s";
}
QPushButton#chipBtn:checked {
  background: rgba(217,168,98,0.18); border: 1px solid %(accent)s; color: %(accent)s;
}
QSlider::groove:horizontal { height: 4px; background: #0a1220; border-radius: 2px; }
QSlider::sub-page:horizontal { background: %(accent)s; border-radius: 2px; }
QSlider::handle:horizontal {
  background: %(accent)s; width: 13px; height: 13px; margin: -5px 0; border-radius: 6px;
}
QLabel#sliderVal { color: %(bright)s; font-size: 11px; font-family: "%(mono)s";
                   font-weight: bold; background: transparent; }
QLabel#swLbl { color: %(muted)s; font-size: 9px; font-family: "%(mono)s"; background: transparent; }
QPushButton#applyBtn {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %(accent)s, stop:1 #b8863f);
  border: 1px solid %(bright)s; border-radius: 10px; color: #0c1626;
  font-weight: bold; font-size: 12px; padding: 7px;
}
QPushButton#applyBtn:hover {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #e6b874, stop:1 #c4934a);
}
QLabel#status { color: %(muted)s; font-size: 10px; font-family: "%(mono)s"; background: transparent; }
""" % {"fg": FG, "bright": BRIGHT, "accent": ACCENT, "muted": MUTED,
       "body": FONT_BODY, "mono": FONT_MONO}
