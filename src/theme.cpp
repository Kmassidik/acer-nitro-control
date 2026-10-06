#include "theme.h"
#include "config.h"

namespace theme {

QString tempColor(int t)
{
    if (t < 72) return "#89b4fa";  // blue
    if (t < 85) return "#f9e2af";  // yellow
    return "#f38ba8";               // red
}

int barTier(int t)
{
    if (t < 72) return 0;
    if (t < 85) return 1;
    return 2;
}

QString stylesheet()
{
    static const char *QSS = R"QSS(
QWidget { background: transparent; color: #cdd6f4; font-family: "%1"; font-size: 13px; }
QFrame#panel {
  background: #1e1e2e;
  border: 1px solid rgba(255,255,255,0.08);
  border-radius: 12px;
}
QLabel#title { color: #cdd6f4; font-size: 13px; font-weight: bold; background: transparent; }
QLabel#dot { color: #a6e3a1; font-size: 12px; background: transparent; }
QLabel#badge {
  background: #313244; color: #cba6f7;
  border: 1px solid rgba(205,214,244,0.12); border-radius: 6px;
  padding: 1px 6px; font-family: "%2"; font-size: 11px;
}
QPushButton#tab {
  background: transparent; border: none; border-radius: 6px 6px 0 0;
  color: #6c7086; font-size: 12px; padding: 5px 10px; font-family: "%2";
}
QPushButton#tab:hover { background: rgba(255,255,255,0.04); color: #cdd6f4; }
QPushButton#tab[active="true"] { background: #313244; color: #cdd6f4; }
QLabel#prompt { color: #cdd6f4; background: transparent; font-family: "%2"; font-size: 13px; }
QLabel#dim { color: #6c7086; background: transparent; }
QLabel#green { color: #a6e3a1; background: transparent; }
QLabel#blue { color: #89b4fa; background: transparent; }
QLabel#teal { color: #94e2d5; background: transparent; }
QLabel#yellow { color: #f9e2af; background: transparent; }
QLabel#red { color: #f38ba8; background: transparent; }
QLabel#term {
  color: #cdd6f4; background: transparent;
  font-family: "%2"; font-size: 13px;
}
QLabel#sect { color: #6c7086; font-size: 11px; background: transparent; font-family: "%2"; }
QLabel#fan { color: #94e2d5; font-size: 12px; background: transparent; font-family: "%2"; }
QLabel#temp { color: #cdd6f4; font-size: 13px; background: transparent; font-family: "%2"; }
QFrame#loadcard { background: transparent; border: none; }
QLabel#load { color: #6c7086; font-size: 12px; font-family: "%2"; background: transparent; }
QLabel#chip { background: transparent; color: #cba6f7; font-size: 12px; font-family: "%2"; }
QFrame#lvlBtn { background: transparent; border: none; border-radius: 4px; }
QFrame#lvlBtn:hover { background: rgba(255,255,255,0.04); }
QFrame#lvlBtn[active="true"] {
  background: #313244; border: 1px solid rgba(255,255,255,0.08); border-radius: 4px;
}
QFrame#lvlBtn[active="true"]:hover { background: #45475a; }
QLabel#btnTitle { color: #9399b2; font-size: 13px; font-family: "%2"; background: transparent; }
QLabel#btnSub { color: #6c7086; font-size: 12px; font-family: "%2"; background: transparent; }
QFrame#lvlBtn[active="true"] QLabel#btnTitle { color: #cdd6f4; font-weight: bold; }
QFrame#lvlBtn[active="true"] QLabel#btnSub { color: #6c7086; }
QLabel#foot { color: #6c7086; font-size: 12px; font-family: "%2"; background: transparent; }
QLabel#footG { color: #a6e3a1; font-size: 12px; font-family: "%2"; background: transparent; }
QPushButton#x { color: #6c7086; border: none; background: transparent; font-size: 16px; padding: 0 4px; }
QPushButton#x:hover { color: #cdd6f4; }
QPushButton#rgbBtn {
  background: #313244; border: 1px solid rgba(205,214,244,0.08);
  border-radius: 6px; color: #cdd6f4; font-size: 12px; padding: 6px 10px;
}
QPushButton#rgbBtn:hover { background: rgba(255,255,255,0.08); }
QFrame#swatch {
  border-radius: 6px; border: 1px solid rgba(255,255,255,0.14); min-width: 42px;
  min-height: 20px; max-height: 20px;
}
QFrame#swatch:hover { border: 1px solid #cba6f7; }
QPushButton#chipBtn {
  background: #313244; border: 1px solid rgba(205,214,244,0.08);
  border-radius: 6px; color: #cdd6f4; font-size: 12px; padding: 4px 9px;
}
QPushButton#chipBtn:checked { background: #cba6f7; color: #1e1e2e; }
QSlider::groove:horizontal { height: 4px; background: #313244; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #cba6f7; border-radius: 2px; }
QSlider::handle:horizontal {
  background: #89b4fa; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px;
}
QLabel#sliderVal { color: #cdd6f4; font-size: 12px; font-family: "%2"; background: transparent; }
QLabel#swLbl { color: #6c7086; font-size: 11px; font-family: "%2"; background: transparent; }
QPushButton#applyBtn {
  background: #cba6f7; border: none; border-radius: 6px;
  color: #1e1e2e; font-weight: bold; font-size: 13px; padding: 6px 10px;
}
QPushButton#applyBtn:hover { background: #b492f0; }
QLabel#status { color: #6c7086; font-size: 12px; font-family: "%2"; background: transparent; }
)QSS";
    return QString(QLatin1String(QSS)).arg(cfg::FONT_MONO, cfg::FONT_MONO);
}

} // namespace theme
