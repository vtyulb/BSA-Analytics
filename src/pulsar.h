#ifndef PULSAR_H
#define PULSAR_H

#include <data.h>
#include <startime.h>
#include <QString>

struct Pulsar {
    Data data;
    int module;
    int ray;
    int dispersion;
    int firstPoint;
    double period; // in seconds;

    QString name; // file, not a pulsar :-)

    double snr;

    bool valid;


    QString print() {
        return QString("Pulsar in %1 module %2, ray %3, D%4, frstp %5 %6, period %7, snr %8").
                arg(name).
                arg(QString::number(module + 1)).
                arg(QString::number(ray + 1)).
                arg(QString::number(dispersion)).
                arg(QString::number(firstPoint)).
                arg(time()).
                arg(QString::number(period)).
                arg(QString::number(snr));
    }

    QString time() {
        return StarTime::StarTime(data, firstPoint);
    }
};


#endif // PULSAR_H
