"""Fedora-glass popup applet: gauges, load line, 6 level buttons.

Lazy-created on first click; its refresh timer runs only while visible.
"""
from PySide6.QtCore import QTimer, Qt
from PySide6.QtWidgets import (QApplication, QFrame, QHBoxLayout, QLabel,
                               QProgressBar, QPushButton, QVBoxLayout, QWidget)

from nitro_tray.config import BADGE, LEVELS, POLL_MS
from nitro_tray.control import read_level, set_level
from nitro_tray.sensors import read_fans, read_temps
from nitro_tray.theme import QSS, bar_tier
from nitro_tray.widgets import LevelButton

WIDTH = 300


class Popup(QWidget):
    def __init__(self, tray):
        super().__init__(None, Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool)
        self.tray = tray
        self.setAttribute(Qt.WA_TranslucentBackground, True)
        self.setStyleSheet(QSS)
        self.setFixedWidth(WIDTH)

        panel = QFrame(self)
        panel.setObjectName("panel")

        # header
        hdr = QHBoxLayout()
        dot = QLabel("●")
        dot.setObjectName("dot")
        title = QLabel("Nitro AN515-58")
        title.setObjectName("title")
        self.badge = QLabel("…")
        self.badge.setObjectName("badge")
        close = QPushButton("×")
        close.setObjectName("x")
        close.clicked.connect(self.hide)
        hdr.addWidget(dot)
        hdr.addWidget(title)
        hdr.addWidget(self.badge)
        hdr.addStretch()
        hdr.addWidget(close)

        # temp rows
        self.cpu_fan = QLabel("—")
        self.cpu_fan.setObjectName("fan")
        self.gpu_fan = QLabel("—")
        self.gpu_fan.setObjectName("fan")
        self.cpu_temp = QLabel("--°C")
        self.cpu_temp.setObjectName("temp")
        self.gpu_temp = QLabel("--°C")
        self.gpu_temp.setObjectName("temp")
        self.cpu_bar = self._bar()
        self.gpu_bar = self._bar()

        # load line
        loadcard = QFrame()
        loadcard.setObjectName("loadcard")
        ll = QHBoxLayout(loadcard)
        ll.setContentsMargins(10, 6, 8, 6)
        self.load_lbl = QLabel("Load: …")
        self.load_lbl.setObjectName("load")
        chip = QLabel("≥88°")
        chip.setObjectName("chip")
        ll.addWidget(self.load_lbl)
        ll.addStretch()
        ll.addWidget(chip)

        # level buttons (3 per row)
        self.btns = {}
        btn_grid = QVBoxLayout()
        btn_grid.setSpacing(7)
        rows = [LEVELS[i:i + 3] for i in range(0, len(LEVELS), 3)]
        for row in rows:
            line = QHBoxLayout()
            line.setSpacing(7)
            for lv in row:
                b = LevelButton(lv["key"], lv["title"], lv["sub"])
                b.clicked.connect(self._pick)
                self.btns[lv["key"]] = b
                line.addWidget(b)
            btn_grid.addLayout(line)

        # footer
        foot = QHBoxLayout()
        for txt, obj in (("hot alert", "foot"), ("≥88°", "foot"),
                         ("· guard:", "foot"), ("nitro-thermal", "footG")):
            lbl = QLabel(txt)
            lbl.setObjectName(obj)
            foot.addWidget(lbl)
        foot.addStretch()

        lay = QVBoxLayout(panel)
        lay.setContentsMargins(16, 14, 16, 14)
        lay.setSpacing(11)
        lay.addLayout(hdr)
        lay.addWidget(self._hline())
        lay.addWidget(self._temp_row("CPU", self.cpu_fan, self.cpu_temp, self.cpu_bar))
        lay.addWidget(self._temp_row("GPU", self.gpu_fan, self.gpu_temp, self.gpu_bar))
        lay.addWidget(loadcard)
        lay.addLayout(btn_grid)

        rgb_btn = QPushButton("Keyboard RGB")
        rgb_btn.setObjectName("rgbBtn")
        rgb_btn.clicked.connect(self.tray._open_rgb)
        lay.addWidget(rgb_btn)
        lay.addLayout(foot)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(panel)

        self.timer = QTimer(self)
        self.timer.setInterval(POLL_MS)
        self.timer.timeout.connect(self.refresh)

    @staticmethod
    def _bar():
        b = QProgressBar()
        b.setRange(0, 100)
        b.setValue(0)
        b.setFormat("")
        b.setProperty("tier", 0)
        return b

    @staticmethod
    def _hline():
        ln = QFrame()
        ln.setFrameShape(QFrame.HLine)
        ln.setFixedHeight(1)
        ln.setStyleSheet("background: rgba(94,110,135,0.35); border: none;")
        return ln

    @staticmethod
    def _temp_row(name, fan, temp, bar):
        w = QWidget()
        wl = QVBoxLayout(w)
        wl.setContentsMargins(0, 0, 0, 0)
        wl.setSpacing(4)
        head = QHBoxLayout()
        head.setSpacing(8)
        lbl = QLabel(name)
        lbl.setObjectName("sect")
        head.addWidget(lbl)
        head.addWidget(fan)
        head.addStretch()
        head.addWidget(temp)
        wl.addLayout(head)
        wl.addWidget(bar)
        return w

    def _pick(self, key):
        set_level(key)
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
        self.badge.setText(BADGE.get(lvl, lvl))
        if len(fans) >= 2:
            self.cpu_fan.setText(f"({fans[0][2]:.0f}%→{fans[0][3]:.0f}%)")
            self.gpu_fan.setText(f"({fans[1][2]:.0f}%→{fans[1][3]:.0f}%)")
            self.load_lbl.setText(f"CPU {fans[0][2]:.0f}%→{fans[0][3]:.0f}%"
                                  f" · GPU {fans[1][2]:.0f}%→{fans[1][3]:.0f}%")
        else:
            self.cpu_fan.setText("(n/a)")
            self.gpu_fan.setText("(n/a)")
            self.load_lbl.setText("Load: n/a")
        for key, b in self.btns.items():
            b.set_active(key == lvl)

    def hideEvent(self, e):
        self.timer.stop()
        super().hideEvent(e)

    def keyPressEvent(self, e):
        k = e.text()
        if k in "12345":
            self._pick(k)
            return
        if k in "aA":
            self._pick("A")
            return
        if e.key() == Qt.Key_Escape:
            self.hide()
            return
        super().keyPressEvent(e)
