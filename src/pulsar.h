#ifndef PULSAR_H
#define PULSAR_H

#include <data.h>
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
        return QString("Pulsar in %1 module %2, ray %3, D%4, frstp %5, period %6, snr %7").
                arg(name).
                arg(QString::number(module)).
                arg(QString::number(ray)).
                arg(QString::number(dispersion)).
                arg(QString::number(firstPoint)).
                arg(QString::number(period)).
                arg(QString::number(snr));
    }
};


#endif // PULSAR_H
