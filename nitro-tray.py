#!/usr/bin/env python3
"""nitro-tray — KDE tray widget: live CPU/GPU temp gauge + popup applet + level switching.

Memory-conscious design:
  - single 3s QTimer (idle cost ~0)
  - 1s pulse QTimer runs ONLY while T >= 88 C
  - popup created lazily on first click; its timer runs only while popup visible
  - icon pixmap rebuilt once per poll only
"""
import re
import subprocess
import sys
from pathlib import Path
from PySide6.QtWidgets import (QApplication, QSystemTrayIcon, QMenu, QMessageBox,
                               QWidget, QLabel, QProgressBar, QPushButton,
                               QHBoxLayout, QVBoxLayout, QGridLayout)
from PySide6.QtGui import (QPixmap, QPainter, QColor, QFont, QIcon, QAction,
                           QActionGroup, QPen)
from PySide6.QtCore import QTimer, QRectF, Qt

POLL_MS = 3000
HOT_C = 88
COOL_C = 85

def read_temps():
    cpu = 0
    for d in Path("/sys/class/hwmon").glob("hwmon*"):
        try:
            if (d / "name").read_text().strip() != "coretemp":
                continue
            for t in d.glob("temp*_input"):
                v = int(t.read_text().strip()) // 1000
                cpu = max(cpu, v)
        except Exception:
            pass
    gpu = 0
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=temperature.gpu",
                              "--format=csv,noheader"], capture_output=True,
                             text=True, timeout=3).stdout.strip().split()[0]
        gpu = int(out)
    except Exception:
        pass
    return cpu, gpu

def read_level():
    try:
        gov = Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor").read_text().strip()
        epp = Path("/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference").read_text().strip()
        turbo = Path("/sys/devices/system/cpu/intel_pstate/no_turbo").read_text().strip()
        guard = subprocess.run(["systemctl", "is-active", "nitro-thermal"],
                               capture_output=True, text=True).stdout.strip()
        if guard == "active":
            return "A"
        combo = (gov, epp, turbo)
        return {
            ("powersave", "power", "1"): "1",
            ("powersave", "balance_power", "1"): "2",
            ("powersave", "balance_performance", "0"): "3",
            ("powersave", "performance", "0"): "4",
            ("performance", "performance", "0"): "5",
        }.get(combo, "?")
    except Exception:
        return "?"

def read_fans():
    """On-demand only (popup visible): parse nbfc status -> [(name, temp, cur%, tgt%)]"""
    try:
        out = subprocess.run(["nbfc", "status"], capture_output=True,
                             text=True, timeout=3).stdout
        fans = re.findall(r"Fan Display Name\s*:\s*([^\n]+)\s+Temperature\s*:\s*([\d.]+)"
                          r".*?Current Fan Speed\s*:\s*([\d.]+)\s+Target Fan Speed\s*:\s*([\d.]+)",
                          out, re.S)
        return [(n.strip(), float(t), float(c), float(g)) for n, t, c, g in fans]
    except Exception:
        return []

def set_level(lvl):
    try:
        subprocess.run(["/home/kurnia/.local/bin/turbo-lvl", lvl],
                       capture_output=True, timeout=30)
    except Exception as e:
        QMessageBox.warning(None, "nitro-tray", f"set level failed: {e}")

def temp_color(t):
    if t < 70:
        return QColor("#2ecc71")
    if t < HOT_C:
        return QColor("#f39c12")
    return QColor("#e74c3c")

def bar_color(t):
    if t < 70:
        return "#2ecc71"
    if t < HOT_C:
        return "#f39c12"
    return "#e74c3c"

def make_icon(t, lvl="?", pulse=False):
    size = 64
    pm = QPixmap(size * 2, size * 2)
    pm.setDevicePixelRatio(2.0)
    pm.fill(QColor(0, 0, 0, 0))
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    col = temp_color(t)
    if pulse:
        col = QColor("#ff5555")
    rect = QRectF(7, 7, 50, 50)
    p.setPen(QPen(QColor("#444444"), 5.5, Qt.SolidLine, Qt.RoundCap))
    p.setBrush(Qt.NoBrush)
    p.drawArc(rect, 0, 360 * 16)
    sweep = -max(0, min(100, t)) / 100 * 360 * 16
    p.setPen(QPen(col, 5.5, Qt.SolidLine, Qt.RoundCap))
    p.drawArc(rect, 90 * 16, int(sweep))
    p.setPen(QColor("white"))
    p.setFont(QFont("sans", 17, QFont.Bold))
    p.drawText(QRectF(0, 0, size, size), int(Qt.AlignHCenter | Qt.AlignVCenter), str(t))
    badge = "!" if pulse else lvl
    p.setBrush(QColor("#1b1b1b"))
    p.setPen(QPen(col, 1.2))
    p.drawRoundedRect(QRectF(22, 50, 20, 13), 5, 5)
    p.setPen(col)
    p.setFont(QFont("sans", 8, QFont.Bold))
    p.drawText(QRectF(22, 50, 20, 13), int(Qt.AlignHCenter | Qt.AlignVCenter), badge)
    p.end()
    return QIcon(pm)

QSS = """
QWidget { background: #1b1b2f; color: #eaeaea; font-size: 12px; }
QLabel#title { font-size: 13px; font-weight: bold; color: #ffffff; }
QLabel#row { color: #c8ccdb; }
QLabel#meta { color: #9aa0b4; font-size: 11px; }
QProgressBar { background: #2b2b40; border: none; border-radius: 7px; height: 16px;
               text-align: center; font-size: 10px; color: #ffffff; }
QProgressBar::chunk { border-radius: 7px; background: #2ecc71; }
QPushButton#lvl { background: #2b2b40; border: 1px solid #3a3a55; border-radius: 9px;
                  padding: 8px 0; font-weight: bold; color: #c8ccdb; }
QPushButton#lvl:checked { background: #4f7cff; border-color: #4f7cff; color: #ffffff; }
QPushButton#lvl:hover { background: #35354d; }
QPushButton#x { background: transparent; border: none; color: #9aa0b4; font-size: 15px;
                padding: 0 6px; }
QPushButton#x:hover { color: #ffffff; }
"""

class Popup(QWidget):
    """Mini status applet: bars + fan line + level buttons. Lives only while visible."""
    def __init__(self, tray):
        super().__init__(None, Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool)
        self.tray = tray
        self.setAttribute(Qt.WA_ShowWithoutActivating, False)
        self.setStyleSheet(QSS)
        self.setFixedWidth(270)

        hdr = QHBoxLayout()
        self.title = QLabel("Nitro AN515-58")
        self.title.setObjectName("title")
        self.lvl_badge = QLabel("A")
        self.lvl_badge.setObjectName("meta")
        x = QPushButton("×")
        x.setObjectName("x")
        x.clicked.connect(self.hide)
        hdr.addWidget(self.title)
        hdr.addWidget(self.lvl_badge)
        hdr.addStretch()
        hdr.addWidget(x)

        self.cpu_bar = QProgressBar(); self.cpu_bar.setRange(0, 100)
        self.gpu_bar = QProgressBar(); self.gpu_bar.setRange(0, 100)
        self.cpu_lbl = QLabel("CPU"); self.cpu_lbl.setObjectName("row")
        self.gpu_lbl = QLabel("GPU"); self.gpu_lbl.setObjectName("row")
        self.fan_lbl = QLabel("fans …"); self.fan_lbl.setObjectName("meta")

        bars = QVBoxLayout()
        for lbl, bar in ((self.cpu_lbl, self.cpu_bar), (self.gpu_lbl, self.gpu_bar)):
            r = QHBoxLayout(); r.addWidget(lbl, 0); r.addWidget(bar, 1)
            bars.addLayout(r)
        bars.addWidget(self.fan_lbl)

        self.btns = {}
        grid = QGridLayout(); grid.setSpacing(6)
        for i, (k, lab) in enumerate([("1", "1 chill"), ("2", "2 cool"),
                                      ("3", "3 game"), ("4", "4 fast"),
                                      ("5", "5 max"), ("A", "Auto")]):
            b = QPushButton(lab)
            b.setObjectName("lvl")
            b.setCheckable(True)
            b.clicked.connect(lambda _=False, kk=k: self._pick(kk))
            self.btns[k] = b
            grid.addWidget(b, i // 3, i % 3)

        foot = QLabel("hot alert ≥88° · guard: nitro-thermal")
        foot.setObjectName("meta")

        lay = QVBoxLayout(self)
        lay.setContentsMargins(14, 12, 14, 12)
        lay.setSpacing(10)
        lay.addLayout(hdr)
        lay.addLayout(bars)
        lay.addLayout(grid)
        lay.addWidget(foot)

        self.timer = QTimer(self)
        self.timer.setInterval(POLL_MS)
        self.timer.timeout.connect(self.refresh)

    def _pick(self, k):
        set_level("auto" if k == "A" else k)
        self.refresh()

    def toggle(self):
        if self.isVisible():
            self.hide()
        else:
            self.refresh()
            self._place()
            self.show()
            self.timer.start()

    def _place(self):
        geo = self.tray.geometry()
        screen = QApplication.screenAt(geo.center()) or QApplication.primaryScreen()
        sg = screen.availableGeometry()
        self.adjustSize()
        x = geo.center().x() - self.width() // 2
        x = max(sg.x() + 4, min(x, sg.x() + sg.width() - self.width() - 4))
        y = geo.y() - self.height() - 8
        if y < sg.y() + 4:
            y = geo.bottom() + 8
        self.move(x, y)

    def refresh(self):
        cpu, gpu = read_temps()
        lvl = read_level()
        for bar, val in ((self.cpu_bar, cpu), (self.gpu_bar, gpu)):
            bar.setValue(val)
            bar.setFormat(f"{val}°C")
            bar.setStyleSheet(f"QProgressBar::chunk {{ background: {bar_color(val)}; }}")
        self.cpu_lbl.setText("CPU")
        self.gpu_lbl.setText("GPU")
        self.lvl_badge.setText({"A": "auto", "1": "lvl1", "2": "lvl2", "3": "lvl3",
                                "4": "lvl4", "5": "lvl5"}.get(lvl, lvl))
        for k, b in self.btns.items():
            b.setChecked(k == lvl)
        fans = read_fans()
        if fans:
            self.fan_lbl.setText("   ·   ".join(
                f"{n.split()[0]} {cur:.0f}%→{tgt:.0f}%" for n, _, cur, tgt in fans))
        else:
            self.fan_lbl.setText("fans: n/a")

    def hideEvent(self, e):
        self.timer.stop()
        super().hideEvent(e)

    def keyPressEvent(self, e):
        if e.key() == Qt.Key_Escape:
            self.hide()
        super().keyPressEvent(e)

class Tray(QSystemTrayIcon):
    def __init__(self):
        super().__init__()
        self.hot = False
        self.pulse_on = False
        self.popup = None  # lazy

        self.menu = QMenu()
        self.info = QAction("…")
        self.info.setEnabled(False)
        self.menu.addAction(self.info)
        self.menu.addSeparator()
        self.group = QActionGroup(self.menu)
        self.group.setExclusive(True)
        self.level_actions = {}
        for lvl, label in [("1", "lvl1  chill (silent)"), ("2", "lvl2  cool (quiet)"),
                           ("3", "lvl3  game (balanced)"), ("4", "lvl4  fast (responsive)"),
                           ("5", "lvl5  max (pinned)"), ("A", "auto  (thermal guard)")]:
            a = QAction(label, self.menu)
            a.setCheckable(True)
            a.triggered.connect(lambda _=False, l=lvl: (set_level("auto" if l == "A" else l),
                                                        self.poll()))
            self.group.addAction(a)
            self.menu.addAction(a)
            self.level_actions[lvl] = a
        self.menu.addSeparator()
        openp = QAction("Open panel", self.menu)
        openp.triggered.connect(self._open_popup)
        self.menu.addAction(openp)
        q = QAction("Quit", self.menu)
        q.triggered.connect(QApplication.quit)
        self.menu.addAction(q)
        self.menu.aboutToShow.connect(self._sync_menu)
        self.setContextMenu(self.menu)
        self.activated.connect(self._on_activated)

        self.timer = QTimer()
        self.timer.timeout.connect(self.poll)
        self.timer.start(POLL_MS)
        self.pulse = QTimer()
        self.pulse.setInterval(500)
        self.pulse.timeout.connect(self._pulse_tick)
        self.poll()
        self.show()

    def _on_activated(self, reason):
        if reason == QSystemTrayIcon.Trigger:
            self._open_popup()
        # Context handled by setContextMenu (right-click menu)

    def _open_popup(self):
        if self.popup is None:
            self.popup = Popup(self)
        self.popup.toggle()

    def _sync_menu(self):
        lvl = read_level()
        for k, a in self.level_actions.items():
            a.setChecked(k == lvl)

    def _pulse_tick(self):
        self.pulse_on = not self.pulse_on
        cpu, gpu = read_temps()
        self.setIcon(make_icon(max(cpu, gpu), read_level(), pulse=self.pulse_on))

    def poll(self):
        cpu, gpu = read_temps()
        lvl = read_level()
        t = max(cpu, gpu)
        self.setIcon(make_icon(t, lvl))
        self.info.setText(f"CPU {cpu}°   GPU {gpu}°   level {lvl}")
        self.setToolTip(f"Nitro thermal — CPU {cpu}°C GPU {gpu}°C (level {lvl})")
        if t >= HOT_C and not self.hot:
            self.hot = True
            self.pulse.start()
        elif t < COOL_C and self.hot:
            self.hot = False
            self.pulse.stop()
            self.pulse_on = False

def main():
    from PySide6.QtCore import QLockFile, QCoreApplication
    QCoreApplication.setApplicationName("nitro-tray")
    lock = QLockFile(str(Path.home() / ".nitro-tray.lock"))
    if not lock.tryLock(50):
        print("nitro-tray already running — exiting")
        return 0
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)
    QSystemTrayIcon.isSystemTrayAvailable() or sys.exit("no tray")
    Tray()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
