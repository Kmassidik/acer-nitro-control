#pragma once
#include <QObject>
#include <QString>

class QDBusMessage;
namespace qkeys {
// Registers Qt::Key_XF86KbdBrightnessUp/Down (as emitted by Fn+F9/Fn+F10 with
// Fedora's stock hwdb mapping) as GLOBAL foreign shortcuts in Plasma's
// kglobalaccel — makes them reach this app regardless of focus/Wayland,
// and signals userDataChanged when the user presses them. No root, no grab:
// KDE owns the keys; we only listen over DBus.
class GlobalKeyListener : public QObject
{
    Q_OBJECT
public:
    explicit GlobalKeyListener(QObject *parent = nullptr);
    bool registerKeys();   // call once after QApplication; false = no kglobalaccel

private slots:
    void globalPressed(const QString &component, const QString &shortcut,
                       qlonglong ts)
    {
        (void)ts;
        if (component != QLatin1String("nitro_control"))
            return;
        if (shortcut == QLatin1String("KbdIllumUp"))
            emit brightUp();
        else if (shortcut == QLatin1String("KbdIllumDown"))
            emit brightDown();
    }

signals:
    void brightUp();
    void brightDown();
};
} // namespace qkeys
