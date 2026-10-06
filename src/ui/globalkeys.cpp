#include "globalkeys.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

namespace qkeys {

// KGlobalAccel stores keycodes as Qt keys: hard keys are plain codes,
// extended (XF86*) keys get the 0x01000000 high bit.
static constexpr int KBD_ILLUM_UP   = 0x01000000 | 230;   // KEY_KBDILLUMUP
static constexpr int KBD_ILLUM_DOWN = 0x01000000 | 229;   // KEY_KBDILLUMDOWN

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

    // doRegister() creates the /component/nitro_control node; without it
    // setForeignShortcut silently drops.
    auto reg = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel", "/kglobalaccel", "org.kde.KGlobalAccel",
        "doRegister");
    // actionId = [component, uniqueShortcut, componentFriendly, actionFriendly]
    reg.setArguments({QStringList{COMPONENT, "KbdIllumDown", "Nitro Control",
                                 "Keyboard Backlight Down"}});
    const QDBusMessage r1 = bus.call(reg);
    if (!tolerated(r1))
        return false;

    reg.setArguments({QStringList{COMPONENT, "KbdIllumUp", "Nitro Control",
                                 "Keyboard Backlight Up"}});
    const QDBusMessage r2 = bus.call(reg);
    if (!tolerated(r2))
        return false;

    // Foreign shortcuts: kglobalaccel grabs the keys and emits
    // globalShortcutPressed(component, shortcut, ts) — no action object needed.
    auto fsc = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel", "/kglobalaccel", "org.kde.KGlobalAccel",
        "setForeignShortcut");
    fsc.setArguments({QStringList{COMPONENT, "KbdIllumDown", "Nitro Control",
                                 "Keyboard Backlight Down"},
                      qkeys_intlist({KBD_ILLUM_DOWN})});
    if (!tolerated(bus.call(fsc)))
        return false;

    fsc.setArguments({QStringList{COMPONENT, "KbdIllumUp", "Nitro Control",
                                 "Keyboard Backlight Up"},
                      qkeys_intlist({KBD_ILLUM_UP})});
    if (!tolerated(bus.call(fsc)))
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
