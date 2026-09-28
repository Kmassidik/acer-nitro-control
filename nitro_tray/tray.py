"""System tray icon: live gauge, context menu, popup launcher, hot pulse."""
from PySide6.QtCore import QTimer
from PySide6.QtGui import QAction, QActionGroup
from PySide6.QtWidgets import QApplication, QMenu, QSystemTrayIcon

from nitro_tray.config import COOL_C, HOT_C, LEVELS, POLL_MS, PULSE_MS
from nitro_tray.control import read_level, set_level
from nitro_tray.icon import make_icon
from nitro_tray.popup import Popup
from nitro_tray.sensors import read_temps


class Tray(QSystemTrayIcon):
    def __init__(self):
        super().__init__()
        self.hot = False
        self.pulse_on = False
        self.popup = None  # lazy
        self.rgb = None    # lazy

        self._build_menu()
        self.activated.connect(self._on_activated)

        self.timer = QTimer()
        self.timer.timeout.connect(self.poll)
        self.timer.start(POLL_MS)
        self.pulse = QTimer()
        self.pulse.setInterval(PULSE_MS)
        self.pulse.timeout.connect(self._pulse_tick)

        self.poll()
        self.show()

    def _build_menu(self):
        self.menu = QMenu()
        self.info = QAction("…")
        self.info.setEnabled(False)
        self.menu.addAction(self.info)
        self.menu.addSeparator()

        self.group = QActionGroup(self.menu)
        self.group.setExclusive(True)
        self.level_actions = {}
        for lv in LEVELS:
            a = QAction(lv["menu"], self.menu)
            a.setCheckable(True)
            key = lv["key"]
            a.triggered.connect(lambda _=False, k=key: (set_level(k), self.poll()))
            self.group.addAction(a)
            self.menu.addAction(a)
            self.level_actions[key] = a

        self.menu.addSeparator()
        openp = QAction("Open panel", self.menu)
        openp.triggered.connect(self._open_popup)
        self.menu.addAction(openp)
        rgb_a = QAction("RGB panel", self.menu)
        rgb_a.triggered.connect(self._open_rgb)
        self.menu.addAction(rgb_a)
        quit_a = QAction("Quit", self.menu)
        quit_a.triggered.connect(QApplication.quit)
        self.menu.addAction(quit_a)
        self.menu.aboutToShow.connect(self._sync_menu)
        self.setContextMenu(self.menu)

    def _on_activated(self, reason):
        if reason == QSystemTrayIcon.Trigger:
            self._open_popup()

    def _open_popup(self):
        if self.popup is None:
            self.popup = Popup(self)
        self.popup.toggle()

    def _open_rgb(self):
        if self.rgb is None:
            from nitro_tray.rgb_panel import RgbPanel
            self.rgb = RgbPanel()
        self.rgb.show()
        self.rgb.raise_()
        self.rgb.activateWindow()

    def _sync_menu(self):
        lvl = read_level()
        for key, a in self.level_actions.items():
            a.setChecked(key == lvl)

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
