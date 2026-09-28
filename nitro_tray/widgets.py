"""Standalone widgets."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import QFrame, QLabel, QVBoxLayout


class LevelButton(QFrame):
    """Two-line level button; gold-gradient when active. Emits clicked(key)."""
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
        self.t = QLabel(title, self)
        self.t.setObjectName("btnTitle")
        self.t.setAlignment(Qt.AlignCenter)
        self.s = QLabel(sub, self)
        self.s.setObjectName("btnSub")
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
