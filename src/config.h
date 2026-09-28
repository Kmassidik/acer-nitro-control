#pragma once
#include <QDir>

namespace cfg {
inline constexpr int POLL_MS   = 3000;
inline constexpr int PULSE_MS  = 500;
inline constexpr int HOT_C     = 88;
inline constexpr int COOL_C    = 85;

inline const QString FONT_BODY = QStringLiteral("Plus Jakarta Sans, Noto Sans");
inline const QString FONT_MONO = QStringLiteral("JetBrains Mono, Monospace");

inline const QString TURBO_LVL = QDir::homePath() + "/.local/bin/turbo-lvl";
inline const QString LOCK_FILE = QDir::homePath() + "/.nitro-tray.lock";
inline const QString GUARD_UNIT = QStringLiteral("nitro-thermal");
inline const QString RGB_STATE = QDir::homePath() + "/.config/nitro-control/rgb.json";

struct Level {
    const char *key, *title, *sub, *menu, *gov, *epp, *turbo;
};
inline constexpr Level LEVELS[6] = {
    {"1", "1 chill", "Quiet",     "lvl1  chill (silent)",   "powersave", "power",               "1"},
    {"2", "2 cool",  "Cool",      "lvl2  cool (quiet)",     "powersave", "balance_power",       "1"},
    {"3", "3 game",  "Balanced",  "lvl3  game (balanced)",  "powersave", "balance_performance", "0"},
    {"4", "4 fast",  "Boost",     "lvl4  fast (boost)",     "powersave", "performance",         "0"},
    {"5", "5 max",   "Full RPM",  "lvl5  max (full rpm)",   "performance", "performance",       "0"},
    {"A", "Auto",    "Dynamic",   "auto  (thermal guard)",  "", "", ""},
};
}
