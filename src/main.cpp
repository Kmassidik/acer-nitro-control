#include "config.h"
#include "control.h"
#include "icon.h"
#include "popup.h"
#include "rgbpanel.h"
#include "tray.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QLockFile>
#include <QTimer>
#include <cstdio>

static bool saveShot(QWidget &w, const QString &file)
{
    w.show();
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    if (!w.grab().save(file)) {
        std::fprintf(stderr, "nitro-control: cannot save screenshot %s\n",
                     qUtf8Printable(file));
        return false;
    }
    return true;
}

int runSelfTest();   // defined in selftest.cpp

int main(int argc, char **argv)
{
    // Keep no desktopFileName: with an app id Qt 6.11 registers the tray via
    // the host portal, which plasmashell 6.7 does not render. Empty id keeps
    // the legacy StatusNotifierItem path (what plasmashell displays).
    // The tray icon also NEEDS QSystemTrayIcon::show() — registration only
    // happens there (see Tray ctor).

    // headless RGB restore (service / sleep hook)
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "--restore-rgb") == 0) {
            QCoreApplication app(argc, argv);
            QString path;
            for (int j = 1; j + 1 < argc; ++j)
                if (qstrcmp(argv[j], "--state") == 0)
                    path = QString::fromLocal8Bit(argv[j + 1]);
            return control::restoreRgb(path);
        }

    // offscreen UI selftest (verification)
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "--selftest") == 0) {
            QApplication app(argc, argv);
            app.setQuitOnLastWindowClosed(false);
            return runSelfTest();
        }

    // offscreen screenshot mode (verification)
    int shotIdx = -1;
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "--screenshot") == 0)
            shotIdx = i;
    if (shotIdx >= 0) {
        QApplication app(argc, argv);
        app.setQuitOnLastWindowClosed(false);
        const QString what = (shotIdx + 1 < argc)
                                 ? QString::fromLocal8Bit(argv[shotIdx + 1]) : QString();
        const QString file = (shotIdx + 2 < argc)
                                 ? QString::fromLocal8Bit(argv[shotIdx + 2]) : QString();
        if (what == QLatin1String("icon")) {
            if (!icon::makeIcon(61, "3").pixmap(QSize(256, 256)).save(file)) {
                std::fprintf(stderr, "nitro-control: cannot save %s\n", qUtf8Printable(file));
                return 1;
            }
            return 0;
        }
        Tray tray;
        if (what == QLatin1String("rgb")) {
            RgbPanel panel;
            return saveShot(panel, file) ? 0 : 1;
        }
        Popup popup(&tray, [] {});
        popup.refresh();
        popup.toggle(); // also starts refresh/anim timers for the snapshot
        return saveShot(popup, file) ? 0 : 1;
    }

    // normal tray: single instance
    QLockFile lock(cfg::LOCK_FILE);
    lock.setStaleLockTime(0);
    if (!lock.tryLock(50)) {
        std::fprintf(stderr, "nitro-control: another instance is running\n");
        return 0;
    }

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("nitro-control");

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        std::fprintf(stderr, "nitro-control: system tray not available\n");
        return 1;
    }
    Tray tray;
    return app.exec();
}
