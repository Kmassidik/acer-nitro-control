# Customizing nitro-control (human guide)

You don't need to know the whole codebase to change how it looks or behaves.
Everything visual lives in **two files**, everything behavior-related in
**one file**. This guide shows what to edit, where, and how to see the change.

## The 30-second loop

```bash
# 1. edit the file (see below)
# 2. install + restart:
cd ~/Desktop/nitro-control
./install.sh
systemctl --user restart nitro-tray
# 3. click the tray gauge → popup opens with your change
```

Tip: the popup and RGB panel read their stylesheet **once at tray start**,
so always restart after editing `theme.py`.

## Where things live

| You want to change… | File |
|---|---|
| Colors (bg, accent, text, buttons) | `nitro_tray/theme.py` |
| Fonts, timing, thresholds, level names | `nitro_tray/config.py` |
| Popup/RGB panel layout (rows, spacing) | `nitro_tray/popup.py`, `nitro_tray/rgb_panel.py` |
| Button height, panel width | `nitro_tray/widgets.py`, `popup.py`, `rgb_panel.py` |
| Tray gauge icon | `nitro_tray/icon.py` |
| Temperature color tiers | `nitro_tray/theme.py` (`temp_color`, `bar_tier`) |
| RGB effects list / default RGB state | `nitro_tray/rgb_panel.py` (`MODES`), `nitro_tray/rgb.py` (`DEFAULT_STATE`) |
| Fan curve | `nbfc/Acer-Nitro-AN515-58.json` |
| Turbo level → CPU settings mapping | `nitro_tray/config.py` (`LEVELS`) **and** `bin/turbo-lvl` |

---

## 1. Colors

All brand colors are named constants at the top of `theme.py`:

```python
ACCENT = "#d9a862"   # gold — buttons, highlights, icons
BRIGHT = "#f2e8d5"   # bright text (titles, temps)
FG     = "#d8cbb4"   # normal text
MUTED  = "#5e6e87"   # dim text (labels, footer)
```

Change `ACCENT` to e.g. `"#4fc3f7"` (blue) and restart — active buttons,
badges, sliders, the `●` dot, and the gauge ring all follow it.

### The big background

In `theme.py`, inside `QSS`, the main panel:

```css
QFrame#panel {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #17243a, stop:1 #0c1626);
  ...
}
```

- `#17243a` = top color, `#0c1626` = bottom color (dark navy pair).
- Flat color instead of gradient? Use one `stop:` line only.
- Same dark tones also appear at: `#0a1220` (bar track), `rgba(38,54,78,…)`
  (cards/buttons), `#141f33` (gauge badge).

### Active (gold) button gradient

```css
QFrame#lvlBtn[active="true"] {
  background: qlineargradient(..., stop:0 #d9a862, stop:1 #b8863f);
  border: 1px solid #f2e8d5;
}
QFrame#lvlBtn[active="true"] QLabel#btnTitle { color: #0c1626; }  # text on gold
```

### Temperature colors

Three places must stay in sync (they're the same rule written twice —
Python for the icon, CSS for the popup bars):

```python
# theme.py
def temp_color(t):      # used by the tray icon ring
    if t < 60:  return QColor("#d9a862")   # gold
    if t < 75:  return QColor("#e0b579")   # light gold
    if t < 85:  return QColor("#f97316")   # orange
    return QColor("#ef4444")               # red

def bar_tier(t):        # returns 0/1/2/3 → matches QSS [tier=N]
```

```css
/* theme.py QSS — popup bars */
QProgressBar::chunk                     { background: #d9a862; }   /* tier 0 */
QProgressBar[tier="1"]::chunk           { background: #e0b579; }
QProgressBar[tier="2"]::chunk           { background: #f97316; }
QProgressBar[tier="3"]::chunk           { background: #ef4444; }
```

Change a threshold → change it in **both** `temp_color` and `bar_tier`.

---

## 2. Sizes (buttons, panels, spacing)

| What | Where | Notes |
|---|---|---|
| Level button height | `widgets.py` → `self.setFixedHeight(52)` | pixels |
| Button corner radius | `theme.py` → `QFrame#lvlBtn { border-radius: 12px; }` | |
| Button label sizes | `theme.py` → `QLabel#btnTitle { font-size: 11px; }` | sub is 9px |
| Popup width | `popup.py` → `WIDTH = 300` | must fit 3 buttons per row |
| RGB panel width | `rgb_panel.py` → `WIDTH = 320` | |
| Space between rows | `popup.py` → `lay.setSpacing(11)` | panel layout |
| Space between buttons | `popup.py` → `line.setSpacing(7)` | grid |
| Inner margins | `popup.py` → `lay.setContentsMargins(16, 14, 16, 14)` | left top right bottom |
| Progress bar height | `theme.py` → `QProgressBar { height: 16px; ... }` | |
| Tray icon size | `icon.py` → `_SIZE = 64` | already 2× rendered for HiDPI |
| Zone swatch size | `theme.py` → `QFrame#swatch { min-width: 44px; min-height: 26px; }` | |

Example — chunky buttons:

```python
# widgets.py
self.setFixedHeight(60)          # was 52
```
```css
/* theme.py */
QLabel#btnTitle { font-size: 13px; }   /* was 11px */
```

---

## 3. Fonts

```python
# config.py
FONT_BODY = "Plus Jakarta Sans, Noto Sans"
FONT_MONO = "JetBrains Mono, Monospace"   # all numbers use this
```

Install any font (`~/.local/share/fonts/` + `fc-cache -f`) and point these
at it. Font **sizes** are in `theme.py` QSS (`font-size:` per element).

---

## 4. Timing & behavior

```python
# config.py
POLL_MS  = 3000   # icon + popup refresh (ms)
PULSE_MS = 500    # hot-alert flash speed
HOT_C    = 88     # guard trip → red pulse + badge "!"
COOL_C   = 85     # pulse stops below this (hysteresis)
```

These are also mirrored in the guard script `bin/nitro-thermal-guard`
(`T -ge 88` / `T -le 78`) — change both if you want different limits.

---

## 5. Level names & CPU behavior

```python
# config.py — LEVELS tuple
{"key": "3", "title": "3 game", "sub": "Balanced",
 "menu": "lvl3  game (balanced)",
 "sysfs": ("powersave", "balance_performance", "0")},
```

- `title`/`sub` → popup button text · `menu` → right-click text
- `sysfs` = (governor, energy_performance_preference, no_turbo)

**Important:** `bin/turbo-lvl` is what actually applies the sysfs values
(case `3) GOV=... EPP=... TURBO=...`). If you change a combo, change it in
**both** places, or the button will show the wrong active state.

---

## 6. RGB defaults

```python
# rgb.py → DEFAULT_STATE
"brightness": 100, "speed": 4, "mode": 1,
"color": [217, 168, 98],          # FX color (gold)
"zones": [[217, 168, 98]] * 4,    # per-zone static colors
```

```python
# rgb_panel.py → MODES
("0", "Static", "zones"), ("1", "Breath", "pulse"), ...
```

Saved state (what re-applies after reboot/suspend):
`~/.config/nitro-control/rgb.json` — delete it to return to defaults.

---

## 7. Checking your change without guessing

Take a screenshot of a panel headlessly (no window needed):

```bash
PYTHONPATH=~/.local/share/nitro-control python3 - <<'EOF'
import sys
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QRect
from nitro_tray.popup import Popup
a = QApplication(sys.argv)
class T:
    def geometry(self): return QRect(900, 1000, 24, 24)
    def _open_rgb(self): pass
p = Popup(T()); p.refresh(); p.show(); a.processEvents()
p.grab().save("/tmp/popup.png")
print("saved /tmp/popup.png")
EOF
```

Logs when something breaks:

```bash
journalctl --user -u nitro-tray -n 50 --no-pager
```
