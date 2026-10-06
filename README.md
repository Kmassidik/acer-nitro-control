# nitro-control (C++ / Qt6)

Native **C++/Qt6** fan, turbo & RGB control for the **Acer Nitro AN515-58** —
Ghost Terminal KDE system-tray widget, 5 turbo levels + auto thermal guard and
keyboard RGB panel, in a single ~200 KB binary.

![popup](docs/screenshot.png)
![rgb panel](docs/screenshot-rgb.png)

## Why C++?

This repo started as a Python/PySide6 app (in git history) and was rewritten
in C++/Qt6 with identical features and look:

| | PySide6 | C++/Qt6 (this) |
|---|---|---|
| RSS | ~88 MB | ~61 MB (mostly shared Qt libs) |
| PSS (unique) | ~40 MB | **~17 MB** |
| Startup | ~1 s | instant |

## Features

- **5 turbo levels + auto** — `chill / cool / game / fast / max`
  (governor + EPP + turbo) and `auto` = thermal guard
  (≥88 °C emergency drop to lvl1, recovers ≤78 °C)
- **System-tray gauge icon** — live temperature ring (HiDPI), color tiers,
  level badge, red pulse while hot
- **Ghost Terminal popup applet** (left-click) — CPU/GPU temp/RPM terminal bars,
  6 mode rows + `1-6` / `Esc` keys; plain menu on right-click
- **Ghost Terminal RGB panel** — 4 zone colors, 6 effects, speed + brightness
- **RGB survives reboot & suspend** — saved to
  `~/.config/nitro-control/rgb.json`, re-applied at login
  (`nitro-rgb-restore.service`) and on resume (`system-sleep` hook)
- **Smooth fan curve** — nbfc custom curve, 7 °C hysteresis, silent at idle
- **Single instance** — `~/.nitro-tray.lock`, lazy popup/RGB windows,
  timers only while visible/hot

## Build

Fedora deps:

```sh
sudo dnf install qt6-qtbase-devel gcc-c++ cmake make
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Install

```sh
./install.sh
```

Builds, installs to `~/.local/bin/nitro-control`, sets up the nbfc fan curve,
the thermal-guard service and the tray/RGB user services (system parts need
`sudo`). Verify with `systemctl --user status nitro-tray`.

### CLI

```
nitro-control                     run tray (single instance)
nitro-control --restore-rgb       re-apply saved RGB state (headless)
nitro-control --restore-rgb --state FILE
nitro-control --screenshot popup|rgb|icon FILE   offscreen UI snapshot
```

## Layout

```
src/
  config.h          tunables: thresholds, level table, paths
  sensors.*         coretemp / nvidia-smi / nbfc readers
  control.*         read & apply performance level, RGB protocol + state
  theme.*           Ghost Terminal palette + Qt stylesheet
  icon.*            tray gauge icon (QPainter)
  levelbutton.*     terminal mode/row button
  popup.*           Ghost Terminal popup applet (fan control)
  rgbpanel.*        Ghost Terminal RGB panel
  tray.*            system-tray icon, menu, hot pulse
  main.cpp          CLI: tray / --restore-rgb / --screenshot
bin/                turbo-lvl + nitro-thermal-guard helpers
systemd/            user units + system-sleep RGB hook
nbfc/               fan curve config
docs/               screenshots
```

## Customizing

- **Palette / QSS** — `src/theme.cpp`, fonts in `src/config.h`
- **Level table** — `src/config.h` `LEVELS` (keep `bin/turbo-lvl` in sync!)
- **Temps / timing** — `src/config.h` (`HOT_C`, `COOL_C`, `POLL_MS`, `PULSE_MS`)
- **UI verification** —
  `QT_QPA_PLATFORM=offscreen ./build/nitro-control --screenshot popup /tmp/p.png`

## Related

- Machine: Acer Nitro AN515-58, Fedora 44, KDE Plasma (Wayland)

## License

MIT — Kurnia Massidik
