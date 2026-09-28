#pragma once
#include <QIcon>
#include <QString>

namespace icon {
QIcon makeIcon(int t, const QString &lvl = "?", bool pulse = false);
}
