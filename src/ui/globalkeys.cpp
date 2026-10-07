#include "globalkeys.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

namespace qkeys {

// Qt key values, NOT raw evdev codes. Chain of evidence:
//   hwdb: scancode ef → KEY_KBDILLUMUP (evdev 230), f0 → KEY_KBDILLUMDOWN (229)
//   xkbcommon-keysyms.h: evdev 0x0e6 → XF86KbdBrightnessUp, 0x0e5 → ...Down
//   Qt qnamespace.h:     Qt::Key_KeyboardBrightnessUp   = 0x010000b5
//                        Qt::Key_KeyboardBrightnessDown = 0x010000b6
// KDE's own powerdevil binding for these keys is 0x010000b5 — proof the
// platform encoding is the Qt value, not 0x01000000|evdev (which never
// matched a physical press: the Fn+F9/F10-dead bug).
static constexpr int KBD_ILLUM_UP   = 0x010000b5;   // Qt::Key_KeyboardBrightnessUp
static constexpr int KBD_ILLUM_DOWN = 0x010000b6;   // Qt::Key_KeyboardBrightnessDown

static const char *COMPONENT = "nitro_control";

// DBus 'ai' — gdbus sends int arrays; QVariantList marshals as av and gets
// rejected ("No such method ... signature asav").
static QDBusArgument intlist(const std::initializer_list<int> &v)
{
    QDBusArgument a;
    a.beginArray(QMetaType::Int);
    for (int k : v)
        a << k;
    a.endArray();
    return a;
}
static QVariant qkeys_intlist(const std::initializer_list<int> &v)
{
    return QVariant::fromValue(intlist(v));
}

GlobalKeyListener::GlobalKeyListener(QObject *parent) : QObject(parent) {}

bool GlobalKeyListener::registerKeys()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return false;

    // Idempotent: a second registration (Tray ctor + selftest) returns
    // AlreadyExists — that's SUCCESS, we own the keys either way.
    const auto tolerated = [](const QDBusMessage &r) {
        return r.type() != QDBusMessage::ErrorMessage ||
               r.errorName().contains("AlreadyExists");
    };

    // 1. doRegister() creates the /component/nitro_control node + the actions.
    auto reg = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel", "/kglobalaccel", "org.kde.KGlobalAccel",
        "doRegister");
    // actionId = [component, uniqueShortcut, componentFriendly, actionFriendly]
    reg.setArguments({QStringList{COMPONENT, "KbdIllumDown", "Nitro Control",
                                 "Keyboard Backlight Down"}});
    if (!tolerated(bus.call(reg)))
        return false;

    reg.setArguments({QStringList{COMPONENT, "KbdIllumUp", "Nitro Control",
                                 "Keyboard Backlight Up"}});
    if (!tolerated(bus.call(reg)))
        return false;

    // 2. setShortcut() — THE call that assigns keys AND activates the
    // component. setForeignShortcut registered the key but left the component
    // INACTIVE (isActive=false, verified live), so presses never fired: that
    // plus the raw-evdev codes was the whole Fn+F9/F10-dead bug.
    // flags=2 (NoAutoloading): don't persist to the user's shortcut config.
    auto sc = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel", "/kglobalaccel", "org.kde.KGlobalAccel",
        "setShortcut");
    sc.setArguments({QStringList{COMPONENT, "KbdIllumDown", "Nitro Control",
                                 "Keyboard Backlight Down"},
                     qkeys_intlist({KBD_ILLUM_DOWN}), uint(2)});
    if (!tolerated(bus.call(sc)))
        return false;

    sc.setArguments({QStringList{COMPONENT, "KbdIllumUp", "Nitro Control",
                                 "Keyboard Backlight Up"},
                     qkeys_intlist({KBD_ILLUM_UP}), uint(2)});
    if (!tolerated(bus.call(sc)))
        return false;

    // listen
    const QString nodePath = QStringLiteral("/component/") + QLatin1String(COMPONENT);
    bus.connect(QString(), nodePath, "org.kde.kglobalaccel.Component",
                "globalShortcutPressed", this,
                SLOT(globalPressed(QString, QString, qlonglong)));
    bus.connect(QString(), nodePath, "org.kde.kglobalaccel.Component",
                "globalShortcutRepeated", this,
                SLOT(globalPressed(QString, QString, qlonglong)));
    return true;
}

} // namespace qkeys
