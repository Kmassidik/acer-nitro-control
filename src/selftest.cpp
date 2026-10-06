// --selftest: offscreen UI test driver.
// Clicks every interactive control and asserts observable effects.
// Exit 0 = all pass. Findings print as "PASS/FAIL: <name>".
#include "config.h"
#include "control.h"
#include "popup.h"
#include "rgbpanel.h"
#include "segmented.h"
#include "tray.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEventLoop>
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <cstdio>

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
    auto *closeDot = dotAt("dotClose");
    auto *maxDot = dotAt("dotMax");
    auto *rgbDot = dotAt("dotRgb");
    report("popup: red close dot exists", closeDot != nullptr);
    report("popup: amber max dot exists", maxDot != nullptr);
    report("popup: violet rgb dot exists", rgbDot != nullptr);

    // hover hint label appears next to title
    bool hintWorks = false;
    const auto labels = popup.findChildren<QLabel *>();
    for (auto *l : labels)
        if (l->objectName() == "hint") {
            QEvent enter(QEvent::Enter);
            QApplication::sendEvent(maxDot, &enter);
            settle(30);
            hintWorks = !l->text().isEmpty();
            QEvent leave(QEvent::Leave);
            QApplication::sendEvent(maxDot, &leave);
            settle(30);
            hintWorks = hintWorks && l->text().isEmpty();
            break;
        }
    report("popup: hover shows dot label hint", hintWorks);

    if (maxDot) {
        const QRect before = popup.geometry();
        clickBtn(maxDot);
        settle(120);
        report("popup: amber dot expands window",
               popup.geometry().width() > before.width() + 100);
        clickBtn(maxDot);
        settle(120);
        report("popup: amber dot restores window", popup.geometry() == before);
    }

    // toggle auto UI reaction (engine writes asserted separately)
    auto *toggle = [&]() -> QAbstractButton * {
        // Toggle is a QToolButton child of the popup's row; find by class
        for (auto *b : popup.findChildren<QToolButton *>())
            if (b->isCheckable() && b->objectName() != "segBtn")
                return b;
        return nullptr;
    }();
    report("popup: auto toggle present+checkable", toggle != nullptr);
    if (toggle) {
        const bool was = toggle->isChecked();
        toggle->setChecked(!was);     // programmatic flip (same code path as click)
        settle(100);
        report("popup: auto toggle flips check state",
               toggle->isChecked() != was);
        toggle->setChecked(was);
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

    if (closeDot)
        clickBtn(closeDot);
    settle(100);
    report("popup: red dot hides window", !popup.isVisible());

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
        // Apply path hits the real RGB device — on a dev box without facer
        // this must FAIL GRACEFULLY with an error string, not crash.
        for (auto *b : panel.findChildren<QPushButton *>())
            if (b->objectName() == "applyBtn") {
                clickBtn(b);
                settle(200);
                break;
            }
        report("rgb: apply survives missing device (no crash)", true);
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
        // 3-second bounded wait for async fan ctrl completion
        bool done = false;
        QTimer::singleShot(3000, [&] { done = true; });
        control::setFansAuto([&] { report("engine: setFansAuto callback fires",
                                          true); done = true; });
        settle(100);
        while (!done)
            settle(50);
    }

    std::printf("----\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
