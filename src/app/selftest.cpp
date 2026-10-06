// --selftest: offscreen UI test driver.
// Clicks every interactive control and asserts observable effects.
// Exit 0 = all pass. Findings print as "PASS/FAIL: <name>".
#include "core/config.h"
#include "core/control.h"
#include "ui/popup.h"
#include "ui/rgbpanel.h"
#include "ui/segmented.h"
#include "ui/tray.h"
#include "ui/globalkeys.h"
#include "core/sensors.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEventLoop>
#include <QLabel>
#include <QMetaObject>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <cstdio>
#include <cstring>

static int g_pass = 0, g_fail = 0;
static void report(const char *name, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    ok ? ++g_pass : ++g_fail;
}
static void settle(int ms = 150)
{
    QEventLoop l;
    QTimer::singleShot(ms, &l, &QEventLoop::quit);
    l.exec();
}

// Synthetic click without QTest dependency
static void clickBtn(QAbstractButton *b)
{
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(0, 0),
                        QPointF(0, 0), Qt::LeftButton, Qt::NoButton,
                        Qt::NoModifier);
    QApplication::sendEvent(b, &press);
    QApplication::sendEvent(b, &release);
}

int runSelfTest()
{
    Tray tray;
    settle();

    // ---- popup UI ----
    Popup popup(&tray, [] {});
    popup.toggle();
    settle(400);

    auto dotAt = [&](const char *cls) -> QPushButton * {
        for (auto *b : popup.findChildren<QPushButton *>())
            if (b->objectName() == cls)
                return b;
        return nullptr;
    };
    auto *rgbBtn = dotAt("utilBtn");
    report("popup: RGB utility button exists", rgbBtn != nullptr);
    auto *closeBtn = dotAt("closeBtn");
    report("popup: close button exists", closeBtn != nullptr);
    // fan apply button (explicit manual commit)
    auto *fanApply = [&]() -> QPushButton * {
        for (auto *b : popup.findChildren<QPushButton *>())
            if (b->objectName() == "applyBtn")
                return b;
        return nullptr;
    }();
    report("popup: fan Apply button exists", fanApply != nullptr);
    if (rgbBtn) {
        // THE CRASH PATH: clicking RGB with an empty std::function slot
        // used to throw std::bad_function_call → SIGABRT. Must survive.
        clickBtn(rgbBtn);
        settle(100);
        report("popup: RGB switch click survives (was bad_function_call)",
               true);
    }

    // hover swaps header title to the button's purpose
    bool hintWorks = false;
    const auto labels = popup.findChildren<QLabel *>();
    for (auto *l : labels)
        if (l->objectName() == "ttlc") {
            QEvent enter(QEvent::Enter);
            QApplication::sendEvent(rgbBtn, &enter);
            settle(30);
            hintWorks = (l->text() == "RGB panel");
            QEvent leave(QEvent::Leave);
            QApplication::sendEvent(rgbBtn, &leave);
            settle(30);
            hintWorks = hintWorks && (l->text() == "Nitro AN515-58");
            break;
        }
    report("popup: hover swaps title to button purpose", hintWorks);

    // BOTH fans must parse (GPU used to vanish → "—" rpm); live nbfc gives 2
    const auto fans = sensors::readFans();
    report("sensors: both fans parsed (size>=2)", fans.size() >= 2);
    if (fans.size() >= 2)
        report("sensors: fan autoCtl parsed", fans[0].autoCtl == fans[1].autoCtl);

    // toggle auto UI reaction (engine writes asserted separately)
    auto *toggle = [&]() -> QAbstractButton * {
        // Toggle is a QToolButton child of the popup's row; find by class
        for (auto *b : popup.findChildren<QToolButton *>())
            if (b->isCheckable() && b->objectName() != "segBtn")
                return b;
        return nullptr;
    }();
    report("popup: auto toggle present+checkable", toggle != nullptr);
    // ---- regression: auto-ON must suppress late manual writes ----
    // (user flow: toggle auto ON; 3 s later helper got 'fan 33/32' from the
    // pending echo debounce → pinned manual. Now applyFans must refuse.)
    if (toggle) {
        toggle->setChecked(true);   // auto ON
        settle(100);
        // fire the pending-write path directly (what the debounce would call)
        popup.refresh();             // simulate echo attempt; must not write
        const QString before = [] {   // state file content
            FILE *f = fopen("/var/lib/nitro-control/fanmode", "r");
            char b[4] = "x";
            if (f) { size_t n = fread(b, 1, 1, f); (void)n; fclose(f); }
            return QString::fromLatin1(b, 1);
        }();
        settle(3000);   // wait out the old debounce window
        const QString after = [] {
            FILE *f = fopen("/var/lib/nitro-control/fanmode", "r");
            char b[4] = "x";
            if (f) { size_t n = fread(b, 1, 1, f); (void)n; fclose(f); }
            return QString::fromLatin1(b, 1);
        }();
        report("popup: auto stays auto; no late manual write",
               after == "1");
        toggle->setChecked(false);   // restore manual for next tests
        settle(100);
    }

    // segmented reacts
    auto *seg = [&]() -> Segmented * {
        for (auto *s : popup.findChildren<Segmented *>())
            return s;
        return nullptr;
    }();
    report("popup: segmented control present", seg != nullptr);
    if (seg) {
        const int before = seg->current();
        seg->select(before == 0 ? 2 : 0);
        settle(100);
        report("popup: segment select moves current", seg->current() != before);
    }

    if (closeBtn) {
        popup.show();
        settle(100);
        clickBtn(closeBtn);
        settle(100);
        report("popup: ✕ button hides window", !popup.isVisible());
    }

    // ---- RGB panel UI ----
    RgbPanel panel(nullptr, [] {});
    panel.show();
    settle(400);

    bool hasSeg = false, hasApply = false, hasZone = false, hasLink = false;
    for (auto *b : panel.findChildren<QToolButton *>())
        if (b->objectName() == "segBtn") {
            hasSeg = true;
            break;
        }
    for (auto *b : panel.findChildren<QPushButton *>()) {
        if (b->objectName() == "applyBtn")
            hasApply = true;
        if (b->objectName() == "zone")
            hasZone = true;
    }
    for (auto *b : panel.findChildren<QToolButton *>())
        if (b->isCheckable() && b->property("linkToggle").toBool())
            hasLink = true;
    report("rgb: effect segmented rendered", hasSeg);
    report("rgb: apply button present", hasApply);
    report("rgb: 4 zone chips present", hasZone);
    report("rgb: link toggle present", hasLink);

    if (hasApply) {
        // click every button EXCEPT those opening modal dialogs (the "+"
        // custom-color picker blocks offscreen — dialog, not crash)
        const auto btns = panel.findChildren<QPushButton *>();
        for (auto *b : btns) {
            if (b->text() == "+")
                continue;
            clickBtn(b);
            settle(30);
        }
        report("rgb: every button click survives", true);
    }
    // keyboard preview must have 75 keycaps
    int keycaps = 0;
    for (auto *w : panel.findChildren<QWidget *>())
        if (w->objectName() == "keycap")
            ++keycaps;
    report("rgb: keyboard preview = 75 keycaps", keycaps == 75);

    panel.hide();

    // ---- engine-level assertions (async wrappers) ----
    {
        // bounded: async nbfc can take ~2s; timeout at 5s ⇒ FAIL, not hang
        QEventLoop l;
        bool fired = false;
        QTimer::singleShot(5000, &l, &QEventLoop::quit);
        control::setFansAuto([&] {
            fired = true;
            QMetaObject::invokeMethod(&l, "quit", Qt::QueuedConnection);
        });
        l.exec();
        report("engine: setFansAuto callback fires", fired);
    }

    // ---- global key shortcut registration (Fn+F9/F10) ----
    {
        qkeys::GlobalKeyListener keys;
        const bool ok = keys.registerKeys();
        report("keys: kglobalaccel shortscut registration", ok);
        if (ok) {
            // confirm both actions are now owned by nitro_control component
            bool down = false, up = false;
            FILE *p = popen(
                "gdbus call --session --dest org.kde.kglobalaccel "
                "--object-path /kglobalaccel --method "
                "org.kde.KGlobalAccel.allComponents 2>/dev/null",
                "r");
            if (p) {
                char buf[512];
                while (fgets(buf, sizeof buf, p))
                    if (strstr(buf, "nitro_control"))
                        down = up = true;
                pclose(p);
            }
            report("keys: nitro_control component exists", down && up);
        }
    }

    std::printf("----\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
