#pragma once
#include <QPair>
#include <QString>
#include <QVector>

namespace sensors {
struct Fan {
    QString name;
    double temp = 0, cur = 0, tgt = 0, steps = 0;
    bool autoCtl = false;   // EC manual-mode bits (0x34/0x33) cleared = auto
};

QPair<int, int> readTemps();          // (cpu, gpu) °C
QVector<Fan> readFans();              // from nitro-priv fan status (EC)
}
