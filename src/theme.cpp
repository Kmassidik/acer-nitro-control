#include "theme.h"
#include "config.h"

namespace theme {

QString tempColor(int t)
{
    if (t < 60) return "#d9a862";
    if (t < 75) return "#e0b579";
    if (t < 85) return "#f97316";
    return "#ef4444";
}

int barTier(int t)
{
    if (t < 60) return 0;
    if (t < 75) return 1;
    if (t < 85) return 2;
    return 3;
}

QString stylesheet()
{
    static const char *QSS = R"QSS(
QWidget { background: transparent; color: #d8cbb4; font-family: "%1"; font-size: 12px; }
QFrame#panel {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #17243a, stop:1 #0c1626);
  border: 1px solid rgba(216,203,180,0.18);
  border-radius: 20px;
}
QLabel#title { color: #f2e8d5; font-size: 13px; font-weight: bold; background: transparent; }
QLabel#dot { color: #d9a862; font-size: 12px; background: transparent; }
QLabel#badge {
  background: rgba(217,168,98,0.14); color: #d9a862;
  border: 1px solid rgba(217,168,98,0.45); border-radius: 6px;
  padding: 1px 7px; font-family: "%2"; font-size: 10px; font-weight: bold;
}
QLabel#sect { color: #d8cbb4; font-size: 11px; font-weight: 600; background: transparent; }
QLabel#fan { color: #5e6e87; font-size: 10px; font-family: "%2"; background: transparent; }
QLabel#temp { color: #f2e8d5; font-size: 13px; font-weight: bold;
              font-family: "%2"; background: transparent; }
QProgressBar {
  background: #0a1220; border: 1px solid rgba(94,110,135,0.5);
  border-radius: 8px; height: 16px; text-align: right; padding-right: 7px;
  color: #f2e8d5; font-family: "%2"; font-size: 9px;
}
QProgressBar::chunk { border-radius: 7px; background: #d9a862; }
QProgressBar[tier="1"]::chunk { background: #e0b579; }
QProgressBar[tier="2"]::chunk { background: #f97316; }
QProgressBar[tier="3"]::chunk { background: #ef4444; }
QFrame#loadcard {
  background: rgba(38,54,78,0.55); border: 1px solid rgba(94,110,135,0.35);
  border-radius: 10px;
}
QLabel#load { color: #d8cbb4; font-family: "%2"; font-size: 10px; background: transparent; }
QLabel#chip {
  background: rgba(217,168,98,0.12); color: #d9a862;
  border: 1px solid rgba(217,168,98,0.35); border-radius: 6px;
  padding: 1px 6px; font-family: "%2"; font-size: 10px; font-weight: bold;
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
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #d9a862, stop:1 #b8863f);
  border: 1px solid #f2e8d5;
}
QLabel#btnTitle { color: #d8cbb4; font-size: 11px; font-weight: bold;
                  background: transparent; border: none; }
QLabel#btnSub { color: #5e6e87; font-size: 9px; font-family: "%2";
                background: transparent; border: none; }
QFrame#lvlBtn[active="true"] QLabel#btnTitle { color: #0c1626; }
QFrame#lvlBtn[active="true"] QLabel#btnSub { color: #4a3c22; }
QLabel#foot { color: #5e6e87; font-size: 10px; font-family: "%2"; background: transparent; }
QLabel#footG { color: #d9a862; font-size: 10px; font-family: "%2"; background: transparent; }
QPushButton#x { color: #5e6e87; border: none; background: transparent; font-size: 15px; padding: 0 4px; }
QPushButton#x:hover { color: #f2e8d5; }
QPushButton#rgbBtn {
  background: rgba(38,54,78,0.55); border: 1px solid rgba(94,110,135,0.45);
  border-radius: 10px; color: #d8cbb4; font-size: 11px; font-weight: 600; padding: 7px;
}
QPushButton#rgbBtn:hover { border-color: rgba(217,168,98,0.6); color: #f2e8d5; }
QFrame#swatch {
  border-radius: 10px; border: 1px solid rgba(216,203,180,0.35); min-width: 44px;
  min-height: 26px; max-height: 26px;
}
QFrame#swatch:hover { border: 1px solid #d9a862; }
QPushButton#chipBtn {
  background: rgba(38,54,78,0.6); border: 1px solid rgba(94,110,135,0.45);
  border-radius: 8px; color: #d8cbb4; font-size: 10px; padding: 4px 9px;
  font-family: "%2";
}
QPushButton#chipBtn:checked {
  background: rgba(217,168,98,0.18); border: 1px solid #d9a862; color: #d9a862;
}
QSlider::groove:horizontal { height: 4px; background: #0a1220; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #d9a862; border-radius: 2px; }
QSlider::handle:horizontal {
  background: #d9a862; width: 13px; height: 13px; margin: -5px 0; border-radius: 6px;
}
QLabel#sliderVal { color: #f2e8d5; font-size: 11px; font-family: "%2";
                   font-weight: bold; background: transparent; }
QLabel#swLbl { color: #5e6e87; font-size: 9px; font-family: "%2"; background: transparent; }
QPushButton#applyBtn {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #d9a862, stop:1 #b8863f);
  border: 1px solid #f2e8d5; border-radius: 10px; color: #0c1626;
  font-weight: bold; font-size: 12px; padding: 7px;
}
QPushButton#applyBtn:hover {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #e6b874, stop:1 #c4934a);
}
QLabel#status { color: #5e6e87; font-size: 10px; font-family: "%2"; background: transparent; }
)QSS";
    return QString(QLatin1String(QSS)).arg(cfg::FONT_BODY).arg(cfg::FONT_MONO);
}

} // namespace theme
