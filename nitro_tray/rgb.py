"""Keyboard RGB via facer char devices + JSON state persistence.

Protocol (from facer_rgb.py / acer-wmi):
  static zone -> /dev/acer-gkbbl-static-0 : [zone_mask, R, G, B]
  mode frame  -> /dev/acer-gkbbl-0        : 16 bytes, frame[9]=1 arms it
Devices are world-writable (no sudo). State file is re-applied at login
(nitro-rgb-restore.service) and after resume (system-sleep hook).
"""
import argparse
import json
import time
from pathlib import Path

DEV = Path("/dev/acer-gkbbl-0")
DEV_STATIC = Path("/dev/acer-gkbbl-static-0")

DEFAULT_STATE = {
    "mode": 1,            # 0 static, 1 breath, 2 neon, 3 wave, 4 shift, 5 zoom
    "speed": 4,           # 1-9
    "brightness": 100,    # 0-100
    "direction": 1,       # 1 right->left, 2 left->right
    "color": [217, 168, 98],           # effects color (gold default)
    "zones": [[217, 168, 98]] * 4,     # static mode, zone 1-4
    "sync": False,
}


def state_path(override=None):
    if override:
        return Path(override)
    return Path.home() / ".config" / "nitro-control" / "rgb.json"


def load_state(path=None):
    p = state_path(path)
    try:
        st = dict(DEFAULT_STATE)
        st.update(json.loads(p.read_text()))
        return st
    except (OSError, ValueError):
        return dict(DEFAULT_STATE)


def save_state(st, path=None):
    p = state_path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(st, indent=2))


def _wait_devices(timeout_s=2.5):
    """Retry while facer module finishes creating nodes (boot/resume edge)."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if DEV.exists() and DEV_STATIC.exists():
            return True
        time.sleep(0.4)
    return DEV.exists() and DEV_STATIC.exists()


def apply(st, save=True, path=None):
    """Write state to the keyboard. Returns (ok, message)."""
    if not _wait_devices():
        return False, "facer device missing (module not loaded?)"
    try:
        mode = int(st.get("mode", 1))
        bright = max(0, min(100, int(st.get("brightness", 100))))
        if mode == 0:
            zones = st.get("zones") or DEFAULT_STATE["zones"]
            for i in range(4):
                r, g, b = (zones[i] if i < len(zones) else zones[0])
                DEV_STATIC.write_bytes(bytes([1 << i, r & 255, g & 255, b & 255]))
            frame = [0] * 16
            frame[2] = bright
            frame[9] = 1
        else:
            speed = max(0, min(255, int(st.get("speed", 4))))
            direction = int(st.get("direction", 1))
            r, g, b = st.get("color") or DEFAULT_STATE["color"]
            frame = [0] * 16
            frame[0] = mode
            frame[1] = speed
            frame[2] = bright
            frame[3] = 8 if mode == 3 else 0
            frame[4] = direction
            frame[5] = r & 255
            frame[6] = g & 255
            frame[7] = b & 255
            frame[9] = 1
        DEV.write_bytes(bytes(frame))
    except OSError as e:
        return False, f"device write failed: {e}"
    if save:
        save_state(st, path)
    return True, "applied"


def restore(path=None):
    st = load_state(path)
    ok, msg = apply(st, save=False, path=path)
    return 0 if ok else 1


def main(argv=None):
    ap = argparse.ArgumentParser(prog="nitro-rgb", description="Nitro keyboard RGB")
    ap.add_argument("--restore", action="store_true",
                    help="re-apply saved state (default action)")
    ap.add_argument("--state", help="state file path override")
    args = ap.parse_args(argv)
    return restore(args.state)


if __name__ == "__main__":
    raise SystemExit(main())
