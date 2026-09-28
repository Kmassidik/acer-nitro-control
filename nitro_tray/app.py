"""App entry: single-instance lock, QApplication, tray bootstrap."""
import sys

from PySide6.QtCore import QCoreApplication, QLockFile
from PySide6.QtWidgets import QApplication, QSystemTrayIcon

from nitro_tray.config import LOCK_FILE


def main():
    QCoreApplication.setApplicationName("nitro-tray")
    lock = QLockFile(str(LOCK_FILE))
    if not lock.tryLock(50):
        print("nitro-tray already running — exiting")
        return 0
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)
    if not QSystemTrayIcon.isSystemTrayAvailable():
        sys.exit("no system tray available")
    from nitro_tray.tray import Tray
    Tray()
    return app.exec()
