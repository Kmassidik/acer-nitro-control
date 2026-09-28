#pragma once
#include <QJsonObject>
#include <QString>

namespace control {
QString readLevel();                          // "1".."5", "A" (guard), "?"
void setLevel(const QString &key);            // runs turbo-lvl (blocks)

// keyboard RGB (facer devices + JSON persistence)
QJsonObject loadRgb(const QString &path = QString());
bool applyRgb(const QJsonObject &st, QString *err = nullptr,
              bool save = true, const QString &path = QString());
int restoreRgb(const QString &path = QString());   // CLI: 0 ok, 1 fail
}
