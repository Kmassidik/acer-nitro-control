#include "core/sensors.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>

namespace sensors {

static int readInt(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return 0;
    return QString::fromUtf8(f.readAll()).trimmed().toInt();
}

QPair<int, int> readTemps()
{
    int cpu = 0;
    const QDir hw("/sys/class/hwmon");
    for (const QString &d : hw.entryList({"hwmon*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile name(hw.filePath(d + "/name"));
        if (!name.open(QIODevice::ReadOnly))
            continue;
        if (QString::fromUtf8(name.readAll()).trimmed() != "coretemp")
            continue;
        const QDir dd(hw.filePath(d));
        for (const QString &t : dd.entryList({"temp*_input"}, QDir::Files))
            cpu = qMax(cpu, readInt(dd.filePath(t)) / 1000);
    }

    int gpu = 0;
    QProcess p;
    p.start("nvidia-smi", {"--query-gpu=temperature.gpu", "--format=csv,noheader"});
    if (p.waitForFinished(3000)) {
        const auto out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (!out.isEmpty())
            gpu = out.split('\n').first().trimmed().toInt();
    }
    return {cpu, gpu};
}

QVector<Fan> readFans()
{
    QVector<Fan> fans;
    // EC state via the root helper (ioperm needs root; sudoers whitelists it).
    QProcess p;
    p.start(QStringLiteral("sudo"), {QStringLiteral("-n"),
                                    QStringLiteral("/usr/local/bin/nitro-priv"),
                                    QStringLiteral("fan"), QStringLiteral("status")});
    if (!p.waitForFinished(3000))
        return fans;
    const QStringList parts = QString::fromUtf8(p.readAllStandardOutput())
                                  .trimmed()
                                  .split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3)
        return fans;
    bool okC = false, okG = false, okA = false;
    const unsigned cw = parts[0].toUInt(&okC);
    const unsigned gw = parts[1].toUInt(&okG);
    const int autoBit = parts[2].toInt(&okA);
    if (!okC || !okG || !okA)
        return fans;
    // word duty-of-8500 → percent (same scale the EC tach reports)
    Fan cpu, gpu;
    cpu.name = QStringLiteral("CPU Fan");
    cpu.cur = cw * 100.0 / 8500.0;
    cpu.tgt = cpu.cur;                       // manual duty == achieved duty
    cpu.steps = 8500;
    cpu.autoCtl = autoBit != 0;
    gpu.name = QStringLiteral("GPU Fan");
    gpu.cur = gw * 100.0 / 8500.0;
    gpu.tgt = gpu.cur;
    gpu.steps = 8500;
    gpu.autoCtl = autoBit != 0;
    fans.append(cpu);
    fans.append(gpu);
    return fans;
}

} // namespace sensors
