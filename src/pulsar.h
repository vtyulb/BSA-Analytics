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

    bool valid;

    QString print() {
        return QString("Pulsar in module %1, ray %2, dispersion %3, frstp %4, period %5").
                arg(QString::number(module)).
                arg(QString::number(ray)).
                arg(QString::number(dispersion)).
                arg(QString::number(firstPoint)).
                arg(QString::number(period));
    }
};


#endif // PULSAR_H
