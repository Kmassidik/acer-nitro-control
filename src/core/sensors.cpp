#include "core/sensors.h"

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
    // Line-based parse — one giant lazy regex can't handle repeated blocks
    // (backtracking swallows the GPU fan into match #1 → size()==1 → "—").
    Fan cur;
    bool inFan = false;
    const auto lines = QString::fromUtf8(p.readAllStandardOutput())
                           .split('\n', Qt::SkipEmptyParts);
    auto val = [](const QString &l, const char *key) -> QString {
        const qsizetype k = l.indexOf(key);
        if (k < 0)
            return {};
        return QStringView(l).mid(k + qstrlen(key)).toString()
                   .mid(l.indexOf(':', k) + 1)
                   .trimmed();
    };
    for (const QString &line : lines) {
        if (line.startsWith("Fan Display Name")) {
            if (inFan)
                fans.append(cur);
            cur = Fan{};
            cur.name = val(line, "Fan Display Name");
            inFan = true;
            continue;
        }
        if (!inFan)
            continue;
        if (line.startsWith("Temperature"))
            cur.temp = val(line, "Temperature").toDouble();
        else if (line.startsWith("Current Fan Speed"))
            cur.cur = val(line, "Current Fan Speed").toDouble();
        else if (line.startsWith("Target Fan Speed"))
            cur.tgt = val(line, "Target Fan Speed").toDouble();
        else if (line.startsWith("Fan Speed Steps"))
            cur.steps = val(line, "Fan Speed Steps").toDouble();
        else if (line.startsWith("Auto Control Enabled"))
            cur.autoCtl = val(line, "Auto Control Enabled")
                              .compare(QLatin1String("true"),
                                       Qt::CaseInsensitive) == 0;
    }
    if (inFan)
        fans.append(cur);
    return fans;
}

} // namespace sensors
