#!/usr/bin/env python3
"""nitro-tray — KDE tray widget: live gauge icon + Fedora-glass popup applet.

Style: Fedora Plasma glass (navy #0c1626 + gold #d9a862), Plus Jakarta Sans +
JetBrains Mono. Memory-conscious: one 3s timer idle; pulse timer only while
T >= 88 C; popup lazy, its timer runs only while visible.
"""
import re
import subprocess
import sys
from pathlib import Path
from PySide6.QtWidgets import (QApplication, QSystemTrayIcon, QMenu, QMessageBox,
                               QWidget, QLabel, QProgressBar, QPushButton,
                               QHBoxLayout, QVBoxLayout, QGridLayout, QFrame)
from PySide6.QtGui import (QPixmap, QPainter, QColor, QFont, QIcon, QAction,
                           QActionGroup, QPen)
from PySide6.QtCore import QTimer, QRectF, Qt, Signal

POLL_MS = 3000
HOT_C = 88
COOL_C = 85

FONT = "Plus Jakarta Sans, Noto Sans"
MONO = "JetBrains Mono, Monospace"

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
    if t < 60:
        return QColor("#d9a862")
    if t < 75:
        return QColor("#e0b579")
    if t < 85:
        return QColor("#f97316")
    return QColor("#ef4444")

def bar_tier(t):
    if t < 60:
        return 0
    if t < 75:
        return 1
    if t < 85:
        return 2
    return 3

def make_icon(t, lvl="?", pulse=False):
    size = 64
    pm = QPixmap(size * 2, size * 2)
    pm.setDevicePixelRatio(2.0)
    pm.fill(QColor(0, 0, 0, 0))
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    col = QColor("#ef4444") if pulse else temp_color(t)
    rect = QRectF(7, 7, 50, 50)
    p.setPen(QPen(QColor("#444444"), 5.5, Qt.SolidLine, Qt.RoundCap))
    p.setBrush(Qt.NoBrush)
    p.drawArc(rect, 0, 360 * 16)
    sweep = -max(0, min(100, t)) / 100 * 360 * 16
    p.setPen(QPen(col, 5.5, Qt.SolidLine, Qt.RoundCap))
    p.drawArc(rect, 90 * 16, int(sweep))
    p.setPen(QColor("#f2e8d5"))
    p.setFont(QFont("JetBrains Mono", 16, QFont.Bold))
    p.drawText(QRectF(0, 0, size, size), int(Qt.AlignHCenter | Qt.AlignVCenter), str(t))
    badge = "!" if pulse else lvl
    p.setBrush(QColor("#141f33"))
    p.setPen(QPen(col, 1.2))
    p.drawRoundedRect(QRectF(22, 50, 20, 13), 5, 5)
    p.setPen(col)
    p.setFont(QFont("JetBrains Mono", 7, QFont.Bold))
    p.drawText(QRectF(22, 50, 20, 13), int(Qt.AlignHCenter | Qt.AlignVCenter), badge)
    p.end()
    return QIcon(pm)

QSS = """
QWidget { background: transparent; color: #d8cbb4; font-family: "%(f)s"; font-size: 12px; }
QFrame#panel {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #17243a, stop:1 #0c1626);
  border: 1px solid rgba(216,203,180,0.18);
  border-radius: 20px;
}
QLabel#title { color: #f2e8d5; font-size: 13px; font-weight: bold; background: transparent; }
QLabel#dot { color: #d9a862; font-size: 12px; background: transparent; }
QLabel#badge {
  background: rgba(217,168,98,0.14); color: #d9a862;
  border: 1px solid rgba(217,168,98,0.45); border-radius: 6px;
  padding: 1px 7px; font-family: "%(m)s"; font-size: 10px; font-weight: bold;
}
QLabel#sect { color: #d8cbb4; font-size: 11px; font-weight: 600; background: transparent; }
QLabel#fan { color: #5e6e87; font-size: 10px; font-family: "%(m)s"; background: transparent; }
QLabel#temp { color: #f2e8d5; font-size: 13px; font-weight: bold;
              font-family: "%(m)s"; background: transparent; }
QProgressBar {
  background: #0a1220; border: 1px solid rgba(94,110,135,0.5);
  border-radius: 8px; height: 16px; text-align: right; padding-right: 7px;
  color: #f2e8d5; font-family: "%(m)s"; font-size: 9px;
}
QProgressBar::chunk { border-radius: 7px; background: #d9a862; }
QProgressBar[tier="1"]::chunk { background: #e0b579; }
QProgressBar[tier="2"]::chunk { background: #f97316; }
QProgressBar[tier="3"]::chunk { background: #ef4444; }
QFrame#loadcard {
  background: rgba(38,54,78,0.55); border: 1px solid rgba(94,110,135,0.35);
  border-radius: 10px;
}
QLabel#load { color: #d8cbb4; font-family: "%(m)s"; font-size: 10px; background: transparent; }
QLabel#chip {
  background: rgba(217,168,98,0.12); color: #d9a862;
  border: 1px solid rgba(217,168,98,0.35); border-radius: 6px;
  padding: 1px 6px; font-family: "%(m)s"; font-size: 10px; font-weight: bold;
}
QFrame#lvlBtn {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(38,54,78,0.75), stop:1 rgba(23,36,58,0.75));
  border: 1px solid rgba(94,110,135,0.5); border-radius: 12px;
}
QFrame#lvlBtn:hover {
  border-color: rgba(217,168,98,0.65);
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(38,54,78,0.95), stop:1 rgba(23,36,58,0.9));
}
QFrame#lvlBtn[active="true"] {
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #d9a862, stop:1 #b8863f);
  border: 1px solid #f2e8d5;
}
QLabel#btnTitle { color: #d8cbb4; font-size: 11px; font-weight: bold;
                  background: transparent; border: none; }
QLabel#btnSub { color: #5e6e87; font-size: 9px; font-family: "%(m)s";
                background: transparent; border: none; }
QFrame#lvlBtn[active="true"] QLabel#btnTitle { color: #0c1626; }
QFrame#lvlBtn[active="true"] QLabel#btnSub { color: #4a3c22; }
QLabel#foot { color: #5e6e87; font-size: 10px; font-family: "%(m)s"; background: transparent; }
QLabel#footG { color: #d9a862; font-size: 10px; font-family: "%(m)s"; background: transparent; }
QLabel#sep { color: #5e6e87; background: transparent; }
QPushButton#x { color: #5e6e87; border: none; background: transparent; font-size: 15px; padding: 0 4px; }
QPushButton#x:hover { color: #f2e8d5; }
""" % {"f": FONT, "m": MONO}

class LevelButton(QFrame):
    clicked = Signal(str)
    def __init__(self, key, title, sub, parent=None):
        super().__init__(parent)
        self.key = key
        self._active = False
        self.setObjectName("lvlBtn")
        self.setCursor(Qt.PointingHandCursor)
        self.setProperty("active", False)
        self.setFixedHeight(52)
        lay = QVBoxLayout(self)
        lay.setContentsMargins(6, 8, 6, 7)
        lay.setSpacing(1)
        self.t = QLabel(title, self); self.t.setObjectName("btnTitle")
        self.s = QLabel(sub, self); self.s.setObjectName("btnSub")
        self.t.setAlignment(Qt.AlignCenter)
        self.s.setAlignment(Qt.AlignCenter)
        lay.addWidget(self.t)
        lay.addWidget(self.s)

    def set_active(self, on):
        if on != self._active:
            self._active = on
            self.setProperty("active", on)
            self.style().unpolish(self)
            self.style().polish(self)

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self.clicked.emit(self.key)
        super().mousePressEvent(e)

class Popup(QWidget):
    """Fedora-glass mini applet: gauges, load line, 6 level buttons. Visible-only cost."""
    def __init__(self, tray):
        super().__init__(None, Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool)
        self.tray = tray
        self.setAttribute(Qt.WA_TranslucentBackground, True)
        self.setStyleSheet(QSS)
        self.setFixedWidth(300)

        panel = QFrame(self)
        panel.setObjectName("panel")

        hdr = QHBoxLayout()
        dot = QLabel("●"); dot.setObjectName("dot")
        title = QLabel("Nitro AN515-58"); title.setObjectName("title")
        self.badge = QLabel("…"); self.badge.setObjectName("badge")
        x = QPushButton("×"); x.setObjectName("x")
        x.clicked.connect(self.hide)
        hdr.addWidget(dot)
        hdr.addWidget(title)
        hdr.addWidget(self.badge)
        hdr.addStretch()
        hdr.addWidget(x)

        self.cpu_fan = QLabel("—"); self.cpu_fan.setObjectName("fan")
        self.gpu_fan = QLabel("—"); self.gpu_fan.setObjectName("fan")
        self.cpu_temp = QLabel("--°C"); self.cpu_temp.setObjectName("temp")
        self.gpu_temp = QLabel("--°C"); self.gpu_temp.setObjectName("temp")
        self.cpu_bar = self._bar()
        self.gpu_bar = self._bar()

        loadcard = QFrame(); loadcard.setObjectName("loadcard")
        ll = QHBoxLayout(loadcard)
        ll.setContentsMargins(10, 6, 8, 6)
        self.load_lbl = QLabel("Load: …"); self.load_lbl.setObjectName("load")
        self.chip = QLabel("≥88°"); self.chip.setObjectName("chip")
        ll.addWidget(self.load_lbl)
        ll.addStretch()
        ll.addWidget(self.chip)

        grid = QGridLayout(); grid.setSpacing(7)
        self.btns = {}
        specs = [("1", "1 chill", "Quiet"), ("2", "2 cool", "Cool"),
                 ("3", "3 game", "Balanced"), ("4", "4 fast", "Boost"),
                 ("5", "5 max", "Full RPM"), ("A", "Auto", "Dynamic")]
        for i, (k, t_, s_) in enumerate(specs):
            b = LevelButton(k, t_, s_)
            b.clicked.connect(self._pick)
            self.btns[k] = b
            grid.addWidget(b, i // 3, i % 3)

        foot = QHBoxLayout()
        f1 = QLabel("hot alert"); f1.setObjectName("foot")
        f2 = QLabel("≥88°"); f2.setObjectName("foot")
        f3 = QLabel("· guard:"); f3.setObjectName("foot")
        f4 = QLabel("nitro-thermal"); f4.setObjectName("footG")
        foot.addWidget(f1); foot.addWidget(f2); foot.addWidget(f3); foot.addWidget(f4)
        foot.addStretch()

        lay = QVBoxLayout(panel)
        lay.setContentsMargins(16, 14, 16, 14)
        lay.setSpacing(11)
        lay.addLayout(hdr)
        lay.addWidget(self._hline())
        lay.addWidget(self._temp_row("CPU", self.cpu_fan, self.cpu_temp, self.cpu_bar))
        lay.addWidget(self._temp_row("GPU", self.gpu_fan, self.gpu_temp, self.gpu_bar))
        lay.addWidget(loadcard)
        lay.addLayout(grid)
        lay.addLayout(foot)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(panel)

        self.timer = QTimer(self)
        self.timer.setInterval(POLL_MS)
        self.timer.timeout.connect(self.refresh)

    def _bar(self):
        b = QProgressBar()
        b.setRange(0, 100)
        b.setValue(0)
        b.setFormat("")
        b.setProperty("tier", 0)
        return b

    def _hline(self):
        ln = QFrame()
        ln.setFrameShape(QFrame.HLine)
        ln.setFixedHeight(1)
        ln.setStyleSheet("background: rgba(94,110,135,0.35); border: none;")
        return ln

    def _temp_row(self, name, fan, temp, bar):
        r = QHBoxLayout()
        r.setSpacing(8)
        lbl = QLabel(name); lbl.setObjectName("sect")
        r.addWidget(lbl)
        r.addWidget(fan)
        r.addStretch()
        r.addWidget(temp)
        r2 = QHBoxLayout()
        r2.addWidget(bar, 1)
        w = QWidget()
        wl = QVBoxLayout(w)
        wl.setContentsMargins(0, 0, 0, 0)
        wl.setSpacing(4)
        wl.addLayout(r)
        wl.addLayout(r2)
        return w

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
        fans = read_fans()
        for bar, val in ((self.cpu_bar, cpu), (self.gpu_bar, gpu)):
            bar.setValue(val)
            tier = bar_tier(val)
            if bar.property("tier") != tier:
                bar.setProperty("tier", tier)
                bar.style().unpolish(bar)
                bar.style().polish(bar)
        self.cpu_temp.setText(f"{cpu}°C")
        self.gpu_temp.setText(f"{gpu}°C")
        self.badge.setText({"A": "auto", "1": "lvl1", "2": "lvl2", "3": "lvl3",
                            "4": "lvl4", "5": "lvl5"}.get(lvl, lvl))
        if len(fans) >= 2:
            self.cpu_fan.setText(f"({fans[0][2]:.0f}%→{fans[0][3]:.0f}%)")
            self.gpu_fan.setText(f"({fans[1][2]:.0f}%→{fans[1][3]:.0f}%)")
            self.load_lbl.setText(f"CPU {fans[0][2]:.0f}%→{fans[0][3]:.0f}%"
                                  f" · GPU {fans[1][2]:.0f}%→{fans[1][3]:.0f}%")
        else:
            self.cpu_fan.setText("(n/a)")
            self.gpu_fan.setText("(n/a)")
            self.load_lbl.setText("Load: n/a")
        for k, b in self.btns.items():
            b.set_active(k == lvl)

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
        self.popup = None

        self.menu = QMenu()
        self.info = QAction("…")
        self.info.setEnabled(False)
        self.menu.addAction(self.info)
        self.menu.addSeparator()
        self.group = QActionGroup(self.menu)
        self.group.setExclusive(True)
        self.level_actions = {}
        for lvl, label in [("1", "lvl1  chill (silent)"), ("2", "lvl2  cool (quiet)"),
                           ("3", "lvl3  game (balanced)"), ("4", "lvl4  fast (boost)"),
                           ("5", "lvl5  max (full rpm)"), ("A", "auto  (thermal guard)")]:
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
