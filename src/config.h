#pragma once
#include <QDir>

namespace cfg {
inline constexpr int POLL_MS   = 3000;
inline constexpr int PULSE_MS  = 500;
inline constexpr int HOT_C     = 88;
inline constexpr int COOL_C    = 85;   // icon pulse stops (guard re-arm is bin/nitro-thermal-guard)

inline const QString FONT_BODY = QStringLiteral("Plus Jakarta Sans, Noto Sans");
inline const QString FONT_MONO = QStringLiteral("JetBrains Mono, Monospace");

inline const QString TURBO_LVL  = QStringLiteral("/usr/local/bin/turbo-lvl");
inline const QString LOCK_FILE  = QDir::homePath() + "/.nitro-tray.lock";
inline const QString GUARD_UNIT = QStringLiteral("nitro-thermal");
inline const QString RGB_STATE  = QDir::homePath() + "/.config/nitro-control/rgb.json";

// Glass Ghost thermal card — 4 manual segments (auto toggle is separate)
struct Level {
    const char *key, *title, *sub, *gov, *epp, *turbo;
};
inline constexpr Level LEVELS[4] = {
    {"1", "Quiet",    "Silent. Light loads.",            "powersave", "power",               "1"},
    {"2", "Balanced", "Good for gaming.",                "powersave", "balance_performance", "0"},
    {"3", "Boost",    "More airflow, more noise.",       "powersave", "performance",         "0"},
    {"4", "Max",      "Full RPM. Loud.",                 "performance", "performance",       "0"},
};
inline constexpr int N_MANUAL = 4;
} // namespace cfg
