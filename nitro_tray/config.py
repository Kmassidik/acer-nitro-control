"""Tunables: poll rates, thresholds, level definitions, paths."""
from pathlib import Path

POLL_MS = 3000
PULSE_MS = 500
HOT_C = 88
COOL_C = 85

FONT_BODY = "Plus Jakarta Sans, Noto Sans"
FONT_MONO = "JetBrains Mono, Monospace"

HOME = Path.home()
TURBO_LVL = HOME / ".local" / "bin" / "turbo-lvl"
LOCK_FILE = HOME / ".nitro-tray.lock"
GUARD_UNIT = "nitro-thermal"

# key/title/sub: popup button · menu: right-click label · sysfs: (gov, epp, no_turbo)
LEVELS = (
    {"key": "1", "title": "1 chill", "sub": "Quiet", "menu": "lvl1  chill (silent)",
     "sysfs": ("powersave", "power", "1")},
    {"key": "2", "title": "2 cool", "sub": "Cool", "menu": "lvl2  cool (quiet)",
     "sysfs": ("powersave", "balance_power", "1")},
    {"key": "3", "title": "3 game", "sub": "Balanced", "menu": "lvl3  game (balanced)",
     "sysfs": ("powersave", "balance_performance", "0")},
    {"key": "4", "title": "4 fast", "sub": "Boost", "menu": "lvl4  fast (boost)",
     "sysfs": ("powersave", "performance", "0")},
    {"key": "5", "title": "5 max", "sub": "Full RPM", "menu": "lvl5  max (full rpm)",
     "sysfs": ("performance", "performance", "0")},
    {"key": "A", "title": "Auto", "sub": "Dynamic", "menu": "auto  (thermal guard)",
     "sysfs": None},
)
SYSFS_TO_KEY = {lv["sysfs"]: lv["key"] for lv in LEVELS if lv["sysfs"]}
BADGE = {lv["key"]: ("auto" if lv["key"] == "A" else "lvl" + lv["key"]) for lv in LEVELS}
