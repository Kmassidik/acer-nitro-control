#include "sensors.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
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
    QProcess p;
    p.start("nbfc", {"status"});
    if (!p.waitForFinished(3000))
        return fans;
    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    static const QRegularExpression re(
        "Fan Display Name\\s*:\\s*([^\\n]+)\\s+Temperature\\s*:\\s*([\\d.]+)"
        ".*?Current Fan Speed\\s*:\\s*([\\d.]+)\\s+Target Fan Speed\\s*:\\s*([\\d.]+)"
        "\\s+Fan Speed Steps\\s*:\\s*(\\d+)",
        QRegularExpression::DotMatchesEverythingOption);
    auto it = re.globalMatch(out);
    while (it.hasNext()) {
        const auto m = it.next();
        fans.append({m.captured(1).trimmed(), m.captured(2).toDouble(),
                     m.captured(3).toDouble(), m.captured(4).toDouble(),
                     m.captured(5).toDouble()});
    }
    return fans;
}

} // namespace sensors
