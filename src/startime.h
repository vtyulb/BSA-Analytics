#ifndef STARTTIME_H
#define STARTTIME_H

#include <QString>
#include <QDateTime>
#include <data.h>

namespace StarTime
{
    QString StarTime(Data data, int point = -1, double *realSeconds = 0);
};

#endif // STARTTIME_H
