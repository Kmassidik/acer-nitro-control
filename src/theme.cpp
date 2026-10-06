#include "theme.h"
#include "config.h"

namespace theme {

QString tempColor(int t)
{
    if (t < 72) return ACCENT;   // violet
    if (t < 85) return WARN;     // amber
    return HOT;                  // red
}

QString stylesheet()
{
    static const char *QSS = R"QSS(
* { outline: none; }
QWidget { background: transparent; color: #e8e6f5; font-family: "%1"; font-size: 13px; }
QWidget:disabled { color: #55516d; }

QFrame#panel {
  background: qradialgradient(cx:0.2, cy:0, radius:1.2, fx:0.2, fy:0,
              stop:0 #2a2540, stop:0.62 #14131c);
  border: 1px solid rgba(255,255,255,0.09);
  border-radius: 20px;
}
QLabel#ttl { color: #8b87a3; font-size: 12px; background: transparent; }
QLabel#ttlc { color: #6f6b8a; font-size: 12px; background: transparent; }
QLabel#dot { background: rgba(255,255,255,0.13); border-radius: 5px; max-width: 10px; max-height: 10px; }
QPushButton#dotClose, QPushButton#dotMin, QPushButton#dotMax, QPushButton#dotRgb {
  border: none; border-radius: 7px; padding: 0; font-size: 9px; font-weight: bold;
  color: transparent;   /* glyph appears on hover, macOS style */
}
QPushButton#dotClose { background: #f87171; }
QPushButton#dotClose:hover { background: #ef4444; color: rgba(0,0,0,0.55); }
QPushButton#dotMin { background: #fbbf24; }
QPushButton#dotMin:hover { background: #d9970c; color: rgba(0,0,0,0.55); }
QPushButton#dotMax { background: #34d399; }
QPushButton#dotMax:hover { background: #10b981; color: rgba(0,0,0,0.55); }
QPushButton#dotRgb { background: transparent; border: none; border-radius: 6px; padding: 0; font-size: 13px; color: #6f6b8a; }
QPushButton#dotRgb:hover { color: #a78bfa; }

QPushButton#utilBtn {
  background: rgba(255,255,255,0.07); border: none; border-radius: 12px;
  padding: 2px 12px; color: #cfcce6; font-size: 12px;
}
QPushButton#utilBtn:hover { background: rgba(255,255,255,0.14); color: #ffffff; }
QLabel#bigTemp { font-size: 46px; font-weight: 600; letter-spacing: -0.03em; background: transparent; }
QLabel#ringLbl { font-size: 11px; color: #8b87a3; letter-spacing: 0.12em; background: transparent; }
QLabel#statLbl { font-size: 11px; color: #8b87a3; background: transparent; }
QLabel#statVal { font-size: 15px; font-weight: 500; color: #e8e6f5; background: transparent; }
QLabel#statUnit { font-size: 10px; color: #8b87a3; background: transparent; }

QFrame#seg {
  background: rgba(255,255,255,0.05);
  border-radius: 12px;
}
QWidget#segOff { opacity: 0.0; }
QToolButton#segBtn {
  background: transparent; border: none; border-radius: 9px;
  color: #9a96b5; font-size: 12px; padding: 8px 6px;
}
QToolButton#segBtn:hover { color: #e8e6f5; }
QToolButton#segBtn:checked { background: rgba(255,255,255,0.11); color: #ffffff; }

QFrame#card { background: transparent; border: none; }
QFrame#hline { background: rgba(255,255,255,0.05); border: none; max-height: 1px; }
QFrame#rowLine { background: rgba(255,255,255,0.05); border: none; max-height: 1px; }

QLabel#row { color: #cfcce6; font-size: 13px; background: transparent; }
QLabel#rowCh { color: #6f6b8a; font-size: 13px; background: transparent; }

QPushButton#toggle {
  background: rgba(255,255,255,0.13); border: none; border-radius: 11px;
  max-width: 36px; max-height: 22px; min-width: 36px; min-height: 22px;
}
QPushButton#toggle::after { content: ""; }
QPushButton#toggle:checked { background: #34d399; }
QPushButton#toggle:hover { border: 1px solid rgba(255,255,255,0.25); }

QPushButton#palBtn {
  border: 2px solid rgba(255,255,255,0.13); border-radius: 13px;
  max-width: 26px; max-height: 26px; min-width: 26px; min-height: 26px; padding: 0;
}
QPushButton#palBtn:hover { border-color: rgba(255,255,255,0.55); }

QFrame#zone {
  border: 2px solid transparent; border-radius: 10px; min-height: 38px;
}
QFrame#zone:hover { border-color: rgba(255,255,255,0.35); }
QFrame#zone[sel="true"] { border-color: #ffffff; }
QLabel#zoneNum { color: rgba(0,0,0,0.62); font-weight: 600; font-size: 11px; background: transparent; }

QFrame#kb {
  background: #0d0c14;
  border: 1px solid rgba(255,255,255,0.07);
  border-radius: 14px;
}
QWidget#keycap { min-width: 13px; min-height: 13px; max-width: 13px; max-height: 13px; }

QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: rgba(255,255,255,0.11); }
QSlider::sub-page:horizontal { background: rgba(255,255,255,0.55); border-radius: 2px; }
QSlider::handle:horizontal {
  background: #ffffff; width: 18px; height: 18px; margin: -7px 0; border-radius: 9px;
}
QSlider::handle:horizontal:hover { background: #ffffff; }
QLabel#lab { font-size: 11px; color: #8b87a3; letter-spacing: 0.08em; background: transparent; }
QLabel#labVal { font-size: 11px; color: #8b87a3; background: transparent; }

QPushButton#desc { background: transparent; color: rgba(255,255,255,0.14); border: none; }
QLabel#status { color: #8b87a3; font-size: 12px; background: transparent; }
)QSS";
    return QString(QLatin1String(QSS)).arg(cfg::FONT_BODY);
}

} // namespace theme
