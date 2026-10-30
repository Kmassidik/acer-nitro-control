#pragma once
#include <functional>
#include <QJsonObject>
#include <QString>

namespace control {
// Level keys are now "1".."4" (manual) and "A" (auto thermal guard); "?" unknown.
QString readLevel();                // blocking (fast sysfs reads + one systemctl call)

// setLevel returns the helper's success (exit code 0); async callbacks carry
// that bool so the UI can toast success/failure honestly.
bool setLevel(const QString &key);  // blocking; UI must call from worker (see setLevelAsync)
void setLevelAsync(const QString &key, std::function<void(bool ok)> instrument);

// Fan duty — direct EC engine via nitro-priv root helper:
//   setFanPct(pct)   manual duty 0..100, both fans (0 = firmware's RPM floor)
//   setFansAuto()    EC firmware curve resumes
// Async (worker thread); callback receives apply success.
void setFanPct(int pct, int fanIndex, std::function<void(bool ok)> instrument);
void setFansAuto(std::function<void(bool ok)> instrument);

// keyboard RGB (facer devices + JSON persistence)
QJsonObject loadRgb(const QString &path = QString());
bool applyRgb(const QJsonObject &st, QString *err = nullptr,
              bool save = true, const QString &path = QString());
int restoreRgb(const QString &path = QString());   // CLI: 0 ok, 1 fail
} // namespace control
