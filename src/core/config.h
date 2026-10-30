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
// `fan` = what the button does to the FAN engine (measured on AN515-58,
// Jimny_ADH BIOS V1.11 — keep honest: RPMs are live-verified values).
struct Level {
    const char *key, *title, *sub, *fan, *gov, *epp, *turbo;
};
inline constexpr Level LEVELS[4] = {
    {"1", "Quiet",    "Cool priority. Light loads.",  "Profile 'quiet'. Curves with load — ~1800 rpm at idle.", "powersave", "power",               "1"},
    {"2", "Balanced", "Everyday + gaming sweet spot.", "Profile 'balanced'. Ramps when CPU/GPU heats up.",       "powersave", "balance_performance", "0"},
    {"3", "Boost",    "More airflow for hits.",       "Turbo fan engine — ~6000+ rpm floor.",                   "powersave", "performance",         "0"},
    {"4", "Max",      "Full RPM. Loud.",              "Turbo fan engine + max CPU — ~7500 rpm.",                "performance", "performance",       "0"},
};
inline constexpr int N_MANUAL = 4;
} // namespace cfg
