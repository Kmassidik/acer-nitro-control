#include "control.h"
#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QCoreApplication>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>
#include <QThreadPool>
#include <chrono>

namespace control {

static QString readTrimmed(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString readLevel()
{
    const QString base = "/sys/devices/system/cpu/";
    const QString gov  = readTrimmed(base + "cpu0/cpufreq/scaling_governor");
    const QString epp  = readTrimmed(base + "cpu0/cpufreq/energy_performance_preference");
    const QString turbo = readTrimmed(base + "intel_pstate/no_turbo");

    QProcess p;
    p.start("systemctl", {"is-active", cfg::GUARD_UNIT});
    if (p.waitForFinished(3000)
        && QString::fromUtf8(p.readAllStandardOutput()).trimmed() == "active")
        return "A";

    for (const auto &lv : cfg::LEVELS) {
        if (gov == lv.gov && epp == lv.epp && turbo == lv.turbo)
            return lv.key;
    }
    return "?";
}

void setLevel(const QString &key)
{
    // "1".."4" manual (old "1".."5" keys collapse to the 4-segment UI),
    // "A" = auto thermal guard.
    const QString arg = (key == "A" || key == "auto")
                            ? QStringLiteral("auto")
                            : (key == "5" ? QStringLiteral("4") : key);
    QProcess p;
    p.start(cfg::TURBO_LVL, {arg});
    p.waitForFinished(-1);
}

void setLevelAsync(const QString &key, std::function<void()> instrument)
{
    // Runs turbo-lvl on a worker thread; QMetaObject::invokeMethod hops the
    // instrument callback back to the caller's (GUI) thread event loop.
    if (auto *ctx = QCoreApplication::instance()) {
        QThreadPool::globalInstance()->start(
            [ctx, key, cb = std::move(instrument)]() mutable {
                setLevel(key);
                QMetaObject::invokeMethod(ctx, [cb = std::move(cb)] { if (cb) cb(); },
                                          Qt::QueuedConnection);
            });
    } else {
        setLevel(key);
        if (instrument) instrument();
    }
}

// ---------------- RGB ----------------
static const char *DEV_MAIN = "/dev/acer-gkbbl-0";
static const char *DEV_STATIC = "/dev/acer-gkbbl-static-0";

static QJsonObject defaultRgb()
{
    QJsonObject st;
    st["mode"] = 1;
    st["speed"] = 4;
    st["brightness"] = 100;
    st["direction"] = 1;
    st["color"] = QJsonArray{217, 168, 98};
    QJsonArray zones;
    for (int i = 0; i < 4; ++i)
        zones.append(QJsonArray{217, 168, 98});
    st["zones"] = zones;
    st["sync"] = false;
    return st;
}

QJsonObject loadRgb(const QString &path)
{
    QFile f(path.isEmpty() ? cfg::RGB_STATE : path);
    if (f.open(QIODevice::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
            QJsonObject st = defaultRgb();
            const QJsonObject up = doc.object();
            for (auto it = up.begin(); it != up.end(); ++it)
                st[it.key()] = it.value();
            return st;
        }
    }
    return defaultRgb();
}

static bool devicesPresent()
{
    return QFile::exists(DEV_MAIN) && QFile::exists(DEV_STATIC);
}

static int jsonInt(const QJsonObject &st, const char *key, int def)
{
    return st.contains(key) ? st[key].toInt(def) : def;
}

bool applyRgb(const QJsonObject &st, QString *err, bool save, const QString &path)
{
    if (!devicesPresent()) {
        if (err) *err = "facer device missing (module not loaded?)";
        return false;
    }
    const int mode = jsonInt(st, "mode", 1);
    const int bright = qBound(0, jsonInt(st, "brightness", 100), 100);

    QByteArray frame(16, '\0');
    if (mode == 0) {
        const QJsonArray zones = st["zones"].toArray();
        for (int i = 0; i < 4; ++i) {
            QJsonArray z = (i < zones.size() && zones[i].isArray())
                               ? zones[i].toArray() : QJsonArray{217, 168, 98};
            const char mask = char(1 << i);
            const char rgb[4] = {mask,
                                 char(z[0].toInt(0) & 255),
                                 char(z[1].toInt(0) & 255),
                                 char(z[2].toInt(0) & 255)};
            QFile fs(DEV_STATIC);
            if (!fs.open(QIODevice::WriteOnly)) {
                if (err) *err = "cannot open " + QString(DEV_STATIC);
                return false;
            }
            fs.write(rgb, 4);
        }
        frame[2] = char(bright);
        frame[9] = 1;
    } else {
        const int speed = qBound(0, jsonInt(st, "speed", 4), 255);
        const int dir = jsonInt(st, "direction", 1);
        const QJsonArray c = st["color"].isArray()
                                 ? st["color"].toArray() : QJsonArray{217, 168, 98};
        frame[0] = char(mode);
        frame[1] = char(speed);
        frame[2] = char(bright);
        frame[3] = (mode == 3) ? 8 : 0;
        frame[4] = char(dir);
        frame[5] = char(c[0].toInt(0) & 255);
        frame[6] = char(c[1].toInt(0) & 255);
        frame[7] = char(c[2].toInt(0) & 255);
        frame[9] = 1;
    }
    QFile fm(DEV_MAIN);
    if (!fm.open(QIODevice::WriteOnly)) {
        if (err) *err = "cannot open " + QString(DEV_MAIN);
        return false;
    }
    if (fm.write(frame) != frame.size()) {
        if (err) *err = "device write failed";
        return false;
    }

    if (save) {
        const QString p = path.isEmpty() ? cfg::RGB_STATE : path;
        QDir().mkpath(QFileInfo(p).absolutePath());
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(QJsonDocument(st).toJson(QJsonDocument::Indented));
    }
    if (err) *err = "applied";
    return true;
}

int restoreRgb(const QString &path)
{
    QString err;
    return applyRgb(loadRgb(path), &err, false, path) ? 0 : 1;
}

} // namespace control
