# nitro-control (C++ / Qt6)

Native **C++/Qt6** fan, turbo & RGB control for the **Acer Nitro AN515-58** —
Glass Ghost KDE system-tray widget, 4 turbo levels + auto thermal guard and
keyboard RGB panel, in a single ~250 KB binary.

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

- **4 turbo levels + auto** — `Quiet / Balanced / Boost / Max`
  (governor + EPP + turbo) and `auto` = thermal guard
  (≥88 °C emergency drop, recovers ≤80 °C with a 180 s hold)
- **System-tray gauge icon** — live temperature ring (HiDPI), color tiers,
  level badge, red pulse while hot
- **Glass Ghost popup** (left-click) — big temp ring, GPU/RPM stats,
  sliding-pill segmented modes, auto-fan toggle; `1-4` / `A` / `Enter` / `Esc` keys
- **Glass Ghost RGB panel** — live animated 15×5 keyboard preview, 4 effects,
  4 zones + 9-color palette + color picker, brightness/speed, link-all-zones
- **RGB survives reboot & suspend** — saved to
  `~/.config/nitro-control/rgb.json`, re-applied at login
  (`nitro-rgb-restore.service`) and on resume (`system-sleep` hook)
- **Smooth fan curve** — nbfc custom curve, 7 °C hysteresis, silent at idle
- **Single instance** — `~/.nitro-tray.lock`, lazy popup/RGB windows,
  timers only while visible/hot

## Security (no stored passwords)

Root work goes through `bin/nitro-priv` — a whitelist helper whose every
effective branch is a fixed literal. `install.sh` installs three **exact-argv
NOPASSWD sudoers lines** (thermal on/off + `cpu *` with token re-validation
inside the helper). No password is stored, echoed or derived anywhere;
`sudo -n` is used by every caller, so a missing rule fails loudly instead of
prompting.

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
nitro-control --selftest                         run the UI test suite (exit 0 = pass)
```

## Layout

Layered — `core` is the engine (no Qt widgets), `ui` is all on-screen stuff,
`app` is the entry point:

```
src/
  core/
    config.h          tunables: thresholds, level table, paths
    sensors.*         coretemp / nvidia-smi / nbfc readers (line parser)
    control.*         apply performance level (async), fan duty via nbfc,
                      RGB protocol + state
  ui/
    theme.*           Glass Ghost palette + Qt stylesheet
    icon.*            tray fan-rotor icon (QPainter)
    segmented.*       sliding-pill segmented control + iOS toggle
    popup.*           Glass Ghost popup (temp ring, fan sliders, modes)
    rgbpanel.*        Glass Ghost RGB panel (live keyboard preview)
    tray.*            system-tray icon, menu, hot pulse
  app/
    main.cpp          CLI: tray / --restore-rgb / --screenshot / --selftest
    selftest.cpp      offscreen UI test driver (17+ assertions)
bin/                  turbo-lvl + nitro-priv (root whitelist) + nitro-thermal-guard
systemd/              user units + system-sleep RGB hook
nbfc/                 fan curve config (Author: Kurnia Massidik)
docs/                 screenshots
```

## Customizing

- **Palette / QSS** — `src/ui/theme.cpp`, fonts in `src/core/config.h`
- **Level table** — `src/core/config.h` `LEVELS` — and keep the token set in
  `bin/nitro-priv` in sync if you add governor/EPP values
- **Temps / timing** — `src/core/config.h` (`HOT_C`, `COOL_C`, `POLL_MS`, `PULSE_MS`)
- **UI verification** —
  `QT_QPA_PLATFORM=offscreen ./build/nitro-control --screenshot popup /tmp/p.png`
  or run the whole UI suite: `QT_QPA_PLATFORM=offscreen ./build/nitro-control --selftest`

## Related

- Machine: Acer Nitro AN515-58, Fedora 44, KDE Plasma (Wayland)

## License

MIT — Kurnia Massidik
