#!/usr/bin/env python3
"""nitro-tray — KDE tray widget: live CPU/GPU temp gauge + turbo level switching.

Memory-conscious design:
  - single 3s QTimer (idle cost ~0)
  - 1s pulse QTimer exists but runs ONLY while T >= 88 C
  - icon pixmap rebuilt once per poll only
"""
import subprocess
import sys
from pathlib import Path
from PySide6.QtWidgets import QApplication, QSystemTrayIcon, QMenu, QMessageBox
from PySide6.QtGui import (QPixmap, QPainter, QColor, QFont, QIcon, QAction,
                           QActionGroup, QPen, QBrush)
from PySide6.QtCore import QTimer, QRectF, Qt

POLL_MS = 3000
HOT_C = 88          # thermal guard threshold
COOL_C = 85         # hysteresis before pulse stops

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
        if gov == "powersave" and epp == "power" and turbo == "1":
            return "1"
        if gov == "performance" and epp == "performance" and turbo == "0":
            return "3"
        return "2"
    except Exception:
        return "?"

def set_level(lvl):
    try:
        subprocess.run(["/home/kurnia/.local/bin/turbo-lvl", lvl],
                       capture_output=True, timeout=30)
    except Exception as e:
        QMessageBox.warning(None, "nitro-tray", f"set level failed: {e}")

def temp_color(t):
    if t < 70:
        return QColor("#2ecc71")   # cool
    if t < HOT_C:
        return QColor("#f39c12")   # warm
    return QColor("#e74c3c")       # hot

def make_icon(t, lvl="?", pulse=False):
    """Gauge-ring icon: arc = temp/100, center = temp, corner badge = level."""
    size = 64
    pm = QPixmap(size * 2, size * 2)          # 2x for HiDPI, logical size 64
    pm.setDevicePixelRatio(2.0)
    pm.fill(QColor(0, 0, 0, 0))
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    col = temp_color(t)
    if pulse:
        col = QColor("#ff5555")                # alert red while guard active
    rect = QRectF(7, 7, 50, 50)
    # background ring
    p.setPen(QPen(QColor("#444444"), 5.5, Qt.SolidLine, Qt.RoundCap))
    p.setBrush(Qt.NoBrush)
    p.drawArc(rect, 0, 360 * 16)
    # temp arc: starts at 12 o'clock, clockwise
    sweep = -max(0, min(100, t)) / 100 * 360 * 16
    p.setPen(QPen(col, 5.5, Qt.SolidLine, Qt.RoundCap))
    p.drawArc(rect, 90 * 16, int(sweep))
    # center number
    p.setPen(QColor("white"))
    f = QFont("sans", 17, QFont.Bold)
    p.setFont(f)
    p.drawText(QRectF(0, 0, size, size), int(Qt.AlignHCenter | Qt.AlignVCenter), str(t))
    # level badge (bottom)
    badge = "!" if pulse else lvl
    p.setBrush(QColor("#1b1b1b"))
    p.setPen(QPen(col, 1.2))
    p.drawRoundedRect(QRectF(22, 50, 20, 13), 5, 5)
    p.setPen(col)
    p.setFont(QFont("sans", 8, QFont.Bold))
    p.drawText(QRectF(22, 50, 20, 13), int(Qt.AlignHCenter | Qt.AlignVCenter), badge)
    p.end()
    return QIcon(pm)

class Tray(QSystemTrayIcon):
    def __init__(self):
        super().__init__()
        self.hot = False
        self.pulse_on = False

        self.menu = QMenu()
        self.info = QAction("…")
        self.info.setEnabled(False)
        self.menu.addAction(self.info)
        self.menu.addSeparator()

        self.group = QActionGroup(self.menu)
        self.group.setExclusive(True)
        self.level_actions = {}
        for lvl, label in [("1", "lvl1  chill (silent)"), ("2", "lvl2  game (balanced)"),
                           ("3", "lvl3  max (pinned)"), ("A", "auto  (thermal guard)")]:
            a = QAction(label, self.menu)
            a.setCheckable(True)
            a.triggered.connect(lambda _=False, l=lvl: (set_level("auto" if l == "A" else l),
                                                        self.poll()))
            self.group.addAction(a)
            self.menu.addAction(a)
            self.level_actions[lvl] = a
        self.menu.addSeparator()
        q = QAction("Quit", self.menu)
        q.triggered.connect(QApplication.quit)
        self.menu.addAction(q)
        self.menu.aboutToShow.connect(self._sync_menu)
        self.setContextMenu(self.menu)
        self.activated.connect(lambda r: self.menu.exec() if r == QSystemTrayIcon.Trigger else None)

        self.timer = QTimer()
        self.timer.timeout.connect(self.poll)
        self.timer.start(POLL_MS)
        # pulse timer: allocated now, started ONLY while hot (idle cost ~0)
        self.pulse = QTimer()
        self.pulse.setInterval(500)
        self.pulse.timeout.connect(self._pulse_tick)
        self.poll()
        self.show()

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
        # hot alert: start/stop pulse with hysteresis
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
