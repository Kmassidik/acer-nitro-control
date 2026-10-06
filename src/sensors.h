#pragma once
#include <QPair>
#include <QString>
#include <QVector>

namespace sensors {
struct Fan {
    QString name;
    double temp = 0, cur = 0, tgt = 0, steps = 0;
    bool autoCtl = false;   // "Auto Control Enabled" from nbfc status
};

QPair<int, int> readTemps();          // (cpu, gpu) °C
QVector<Fan> readFans();              // from `nbfc status`
}
