"""Read/apply the performance level (governor + EPP + turbo via turbo-lvl)."""
import subprocess

from pathlib import Path

from PySide6.QtWidgets import QMessageBox

from nitro_tray.config import SYSFS_TO_KEY, GUARD_UNIT, TURBO_LVL

_CPU = Path("/sys/devices/system/cpu")


def read_level():
    """Return active level key ('1'..'5', 'A' = guard, '?' = unknown)."""
    try:
        gov = (_CPU / "cpu0/cpufreq/scaling_governor").read_text().strip()
        epp = (_CPU / "cpu0/cpufreq/energy_performance_preference").read_text().strip()
        turbo = (_CPU / "intel_pstate/no_turbo").read_text().strip()
        guard = subprocess.run(["systemctl", "is-active", GUARD_UNIT],
                               capture_output=True, text=True, timeout=3).stdout.strip()
        if guard == "active":
            return "A"
        return SYSFS_TO_KEY.get((gov, epp, turbo), "?")
    except (OSError, subprocess.TimeoutExpired):
        return "?"


def set_level(key):
    """Apply a level ('1'..'5' or 'A' = enable thermal guard). Blocks until done."""
    arg = "auto" if key == "A" else key
    try:
        subprocess.run([str(TURBO_LVL), arg], capture_output=True, timeout=30)
    except (OSError, subprocess.TimeoutExpired) as e:
        QMessageBox.warning(None, "nitro-tray", f"set level failed: {e}")
