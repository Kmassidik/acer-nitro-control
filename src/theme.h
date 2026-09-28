#pragma once
#include <QString>

namespace theme {
inline const QString ACCENT = "#d9a862";
inline const QString BRIGHT = "#f2e8d5";
inline const QString FG     = "#d8cbb4";
inline const QString MUTED  = "#5e6e87";

QString stylesheet();          // Fedora-glass QSS (fonts from cfg)
QString tempColor(int t);      // "#rrggbb" tiers: <60 <75 <85 else red
int barTier(int t);            // 0..3, matches QProgressBar[tier=N]
}
