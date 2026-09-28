"""Glass RGB panel: zones, effects, speed/brightness — opens from the tray widget.

Single lazy instance (no per-open recreation, no timers) — zero idle cost.
"""
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (QApplication, QColorDialog, QFrame, QHBoxLayout,
                               QLabel, QPushButton, QSlider, QVBoxLayout,
                               QWidget)

from nitro_tray import rgb
from nitro_tray.theme import QSS
from nitro_tray.widgets import LevelButton

MODES = (
    ("0", "Static", "zones"), ("1", "Breath", "pulse"), ("2", "Neon", "cycle"),
    ("3", "Wave", "flow"), ("4", "Shift", "glide"), ("5", "Zoom", "zoom"),
)

WIDTH = 320


def _hex(c):
    return f"#{c[0]:02x}{c[1]:02x}{c[2]:02x}"


class RgbPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(None, Qt.FramelessWindowHint | Qt.Tool)
        self.setWindowModality(Qt.NonModal)
        self.setAttribute(Qt.WA_TranslucentBackground, True)
        self.setStyleSheet(QSS)
        self.setFixedWidth(WIDTH)
        self.state = rgb.load_state()

        panel = QFrame(self)
        panel.setObjectName("panel")

        # header
        hdr = QHBoxLayout()
        dot = QLabel("●")
        dot.setObjectName("dot")
        title = QLabel("Keyboard RGB")
        title.setObjectName("title")
        close = QPushButton("×")
        close.setObjectName("x")
        close.clicked.connect(self.hide)
        hdr.addWidget(dot)
        hdr.addWidget(title)
        hdr.addStretch()
        hdr.addWidget(close)

        # zone swatches + sync
        zrow = QHBoxLayout()
        zrow.setSpacing(6)
        self.zone_btns = []
        for i in range(4):
            wrap = QVBoxLayout()
            wrap.setSpacing(2)
            sw = QFrame()
            sw.setObjectName("swatch")
            sw.setCursor(Qt.PointingHandCursor)
            sw.mousePressEvent = lambda _e, idx=i: self._pick_zone(idx)
            lbl = QLabel(f"Z{i + 1}")
            lbl.setObjectName("swLbl")
            lbl.setAlignment(Qt.AlignCenter)
            wrap.addWidget(sw)
            wrap.addWidget(lbl)
            zrow.addLayout(wrap)
            self.zone_btns.append(sw)
        self.sync_btn = QPushButton("Sync")
        self.sync_btn.setObjectName("chipBtn")
        self.sync_btn.setCheckable(True)
        self.sync_btn.toggled.connect(self._sync_toggled)
        zrow.addWidget(self.sync_btn, 0, Qt.AlignBottom)

        # fx color
        frow = QHBoxLayout()
        flbl = QLabel("FX color")
        flbl.setObjectName("swLbl")
        self.fx_btn = QFrame()
        self.fx_btn.setObjectName("swatch")
        self.fx_btn.setCursor(Qt.PointingHandCursor)
        self.fx_btn.mousePressEvent = lambda _e: self._pick_fx()
        frow.addWidget(flbl)
        frow.addWidget(self.fx_btn)
        frow.addStretch()

        # modes 3x2
        self.mode_btns = {}
        grid = QVBoxLayout()
        grid.setSpacing(7)
        for row_start in (0, 3):
            line = QHBoxLayout()
            line.setSpacing(7)
            for key, name, sub in MODES[row_start:row_start + 3]:
                b = LevelButton(key, name, sub)
                b.clicked.connect(self._pick_mode)
                self.mode_btns[key] = b
                line.addWidget(b)
            grid.addLayout(line)

        # sliders
        self.speed_val = self._val_lbl()
        self.bright_val = self._val_lbl()
        self.speed = self._slider(1, 9, self.speed_val)
        self.bright = self._slider(0, 100, self.bright_val)

        # apply + status
        apply_btn = QPushButton("Apply")
        apply_btn.setObjectName("applyBtn")
        apply_btn.clicked.connect(self._apply)
        self.status = QLabel("ready")
        self.status.setObjectName("status")
        self.status.setAlignment(Qt.AlignCenter)

        lay = QVBoxLayout(panel)
        lay.setContentsMargins(16, 14, 16, 14)
        lay.setSpacing(11)
        lay.addLayout(hdr)
        lay.addWidget(self._hline())
        lay.addLayout(zrow)
        lay.addLayout(frow)
        lay.addLayout(grid)
        lay.addLayout(self._slider_row("Speed", self.speed, self.speed_val))
        lay.addLayout(self._slider_row("Brightness", self.bright, self.bright_val))
        lay.addWidget(apply_btn)
        lay.addWidget(self.status)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(panel)

    # -- builders ---------------------------------------------------------
    @staticmethod
    def _hline():
        ln = QFrame()
        ln.setFrameShape(QFrame.HLine)
        ln.setFixedHeight(1)
        ln.setStyleSheet("background: rgba(94,110,135,0.35); border: none;")
        return ln

    @staticmethod
    def _val_lbl():
        v = QLabel()
        v.setObjectName("sliderVal")
        return v

    @staticmethod
    def _slider(lo, hi, val_lbl):
        s = QSlider(Qt.Horizontal)
        s.setRange(lo, hi)
        s.valueChanged.connect(lambda v, lbl=val_lbl: lbl.setText(str(v)))
        return s

    @staticmethod
    def _slider_row(name, slider, val_lbl):
        row = QHBoxLayout()
        row.setSpacing(8)
        lbl = QLabel(name)
        lbl.setObjectName("sect")
        row.addWidget(lbl)
        row.addWidget(slider, 1)
        row.addWidget(val_lbl)
        return row

    # -- state <-> ui -----------------------------------------------------
    def showEvent(self, e):
        self.state = rgb.load_state()
        self._load_ui()
        super().showEvent(e)

    def _load_ui(self):
        st = self.state
        for i, sw in enumerate(self.zone_btns):
            self._paint(sw, st["zones"][i])
        self._paint(self.fx_btn, st["color"])
        self.sync_btn.setChecked(bool(st.get("sync")))
        for key, b in self.mode_btns.items():
            b.set_active(int(key) == int(st.get("mode", 1)))
        self.speed.setValue(int(st.get("speed", 4)))
        self.bright.setValue(int(st.get("brightness", 100)))
        self.speed_val.setText(str(self.speed.value()))
        self.bright_val.setText(str(self.bright.value()))

    @staticmethod
    def _paint(frame, color):
        frame.setStyleSheet(
            f"background: {_hex(color)}; border-radius: 10px;"
            "border: 1px solid rgba(216,203,180,0.35);")

    # -- interactions -----------------------------------------------------
    def _pick_zone(self, idx):
        c = QColorDialog.getColor(QColor(*self.state["zones"][idx]), self,
                                  f"Zone {idx + 1}")
        if not c.isValid():
            return
        rgbv = [c.red(), c.green(), c.blue()]
        if self.sync_btn.isChecked():
            self.state["zones"] = [rgbv] * 4
        else:
            self.state["zones"][idx] = rgbv
        for i, sw in enumerate(self.zone_btns):
            self._paint(sw, self.state["zones"][i])

    def _pick_fx(self):
        c = QColorDialog.getColor(QColor(*self.state["color"]), self, "FX color")
        if c.isValid():
            self.state["color"] = [c.red(), c.green(), c.blue()]
            self._paint(self.fx_btn, self.state["color"])

    def _sync_toggled(self, on):
        if on:
            self.state["zones"] = [list(self.state["zones"][0])] * 4
            for i, sw in enumerate(self.zone_btns):
                self._paint(sw, self.state["zones"][i])

    def _pick_mode(self, key):
        self.state["mode"] = int(key)
        for k, b in self.mode_btns.items():
            b.set_active(k == key)

    def _apply(self):
        st = self.state
        st["speed"] = self.speed.value()
        st["brightness"] = self.bright.value()
        st["sync"] = self.sync_btn.isChecked()
        ok, msg = rgb.apply(st)
        self.status.setText("✓ applied" if ok else f"✗ {msg}")
