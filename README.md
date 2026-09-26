# nitro-control — AN515-58 fan + thermal + turbo levels

One folder, everything visible. Live copies run from system paths; edit here, then `bash install.sh` to deploy.

## Files

| File | What | Lives at (running copy) |
|------|------|--------------------------|
| `turbo-lvl` | `turbo-lvl 1\|2\|3\|auto` — 3 CPU levels | `~/.local/bin/turbo-lvl` |
| `nitro-thermal-guard` | daemon: polls CPU+GPU temp every 5s, `>=88C→lvl1`, `<=78C→lvl2` | `/usr/local/bin/nitro-thermal-guard` |
| `nitro-thermal.service` | system unit for the guard | `/etc/systemd/system/nitro-thermal.service` |
| `nitro-tray.py` | KDE tray widget: T° icon + click menu | `~/.local/bin/nitro-tray.py` |
| `nitro-tray.service` | user unit for the tray | `~/.config/systemd/user/nitro-tray.service` |
| `Acer-Nitro-AN515-58.json` | nbfc fan curve (linear controller) | `/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json` |
| `install.sh` | deploys everything + enables autostart | — |

## The 3 levels

- **lvl1 chill**: `powersave/power`, turbo OFF — silent browsing
- **lvl2 game**: `powersave/balance_performance`, turbo ON — Dota
- **lvl3 max**: `performance/performance`, turbo ON — pinned, loud
- **auto**: thermal guard drives 2↔1 from silicon temps

## The math (measured on this machine)

- Light load: package **13.5W** (RAPL) → 54°C → R ≈ 1.0°C/W
- Heavy: ~70W → 74°C → R ≈ 0.5°C/W at ~80% fan
- Controller: `fan% = 2.0 × (T − 55)`, silent <58, safety ramp 88→100
- `EcPollInterval 2000ms`, `MaxSpeedValueRead 8500` (EC reports 7894–8000)

## Autostart (all enabled)

- `nbfc_service` — fan control (system, boot)
- `nitro-thermal` — thermal guard (system, boot)
- `nitro-tray` — tray widget (user graphical session + autostart .desktop)
