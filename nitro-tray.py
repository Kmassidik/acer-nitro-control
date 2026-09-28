#!/usr/bin/env python3
"""nitro-tray — KDE tray widget: live CPU/GPU temp + turbo level switching."""
import subprocess
import sys
from pathlib import Path
from PySide6.QtWidgets import QApplication, QSystemTrayIcon, QMenu, QMessageBox
from PySide6.QtGui import QPixmap, QPainter, QColor, QFont, QIcon, QAction
from PySide6.QtCore import QTimer

POLL_MS = 3000

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
            return "auto"
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

def make_icon(t):
    color = "#2ecc71" if t < 70 else ("#f39c12" if t < 88 else "#e74c3c")
    pm = QPixmap(64, 64)
    pm.fill(QColor("transparent"))
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setBrush(QColor(color))
    p.setPen(QColor(color))
    p.drawRoundedRect(2, 2, 60, 60, 12, 12)
    p.setPen(QColor("white"))
    p.setFont(QFont("sans", 22, QFont.Bold))
    p.drawText(pm.rect(), 0x84, str(t))  # center
    p.end()
    return QIcon(pm)

class Tray(QSystemTrayIcon):
    def __init__(self):
        super().__init__()
        self.menu = QMenu()
        self.info = QAction("…")
        self.info.setEnabled(False)
        self.menu.addAction(self.info)
        self.menu.addSeparator()
        for lvl, label in [("1", "lvl1 chill (silent)"), ("2", "lvl2 game (balanced)"),
                           ("3", "lvl3 max (pinned)"), ("auto", "auto (thermal guard)")]:
            a = QAction(label, self.menu)
            a.triggered.connect(lambda _=False, l=lvl: (set_level(l), self.poll()))
            self.menu.addAction(a)
        self.menu.addSeparator()
        q = QAction("Quit", self.menu)
        q.triggered.connect(QApplication.quit)
        self.menu.addAction(q)
        self.setContextMenu(self.menu)
        self.activated.connect(lambda r: self.menu.exec_() if r == QSystemTrayIcon.Trigger else None)
        self.timer = QTimer()
        self.timer.timeout.connect(self.poll)
        self.timer.start(POLL_MS)
        self.poll()
        self.show()

    def poll(self):
        cpu, gpu = read_temps()
        lvl = read_level()
        t = max(cpu, gpu)
        self.setIcon(make_icon(t))
        self.info.setText(f"T cpu {cpu}° gpu {gpu}° | lvl {lvl}")
        self.setToolTip(f"Nitro thermal — CPU {cpu}°C GPU {gpu}°C (lvl {lvl})")

def main():
    # single-instance guard: never register the tray icon twice
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
