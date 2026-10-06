#pragma once
#include <functional>
#include <QJsonObject>
#include <QString>

namespace control {
// Level keys are now "1".."4" (manual) and "A" (auto thermal guard); "?" unknown.
QString readLevel();                // blocking (fast sysfs reads + one systemctl call)
void setLevel(const QString &key);  // blocking; UI must call from worker (see setLevelAsync)

// Async wrapper: runs setLevel off the GUI thread, then instruments(key) on it.
void setLevelAsync(const QString &key, std::function<void()> instrument);

// Fan duty (DAM-FC engine port, driven through nbfc):
//   setFanPct(pct, fanIndex)  manual duty 0..100 for fan 0/1 (-1 = all)
//   setFansAuto()             nbfc -a (profile curve / guard takes over)
// Both off-GUI-thread; nbfc client is slow (unit holds ~2 s).
void setFanPct(int pct, int fanIndex, std::function<void()> instrument);
void setFansAuto(std::function<void()> instrument);

// keyboard RGB (facer devices + JSON persistence)
QJsonObject loadRgb(const QString &path = QString());
bool applyRgb(const QJsonObject &st, QString *err = nullptr,
              bool save = true, const QString &path = QString());
int restoreRgb(const QString &path = QString());   // CLI: 0 ok, 1 fail
} // namespace control
