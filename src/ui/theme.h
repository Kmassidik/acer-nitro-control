#pragma once
#include <QString>

namespace theme {
// Glass Ghost palette (ported from the HTML mock)
inline const QString BG       = "#0b0b12";
inline const QString PANEL    = "#14131c";
inline const QString PANEL_HI = "#2a2540";
inline const QString TX       = "#e8e6f5";
inline const QString MUTED    = "#8b87a3";
inline const QString DIM      = "#6f6b8a";
inline const QString LINE     = "#ffffff12";
inline const QString OK       = "#34d399";
inline const QString ACCENT   = "#a78bfa";   // violet
inline const QString WARN     = "#fbbf24";
inline const QString HOT      = "#f87171";

QString stylesheet();          // Glass Ghost QSS (fonts from cfg)
QString tempColor(int t);      // violet <72, amber <85, red >=85
} // namespace theme
