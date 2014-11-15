#ifndef PULSAR_H
#define PULSAR_H

#include <data.h>
#include <startime.h>
#include <QString>
#include <QVector>
#include <QVariant>

const int INTERVAL = 5;
const double MINIMUM_PERIOD = 0.5;
const double MAXIMUM_PERIOD = 10;
const double PERIOD_STEP = 0.01;
const int interval = 180;

struct Pulsar {
    Data data;
    int module;
    int ray;
    int dispersion;
    int firstPoint;
    double period; // in seconds;
    double snr;

    double noiseLevel;

    QString name; // file, not a pulsar :-)
    QByteArray additionalData;
    bool valid;
    QTime nativeTime;

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

    void calculateAdditionalData(const QVector<double> &disp) {
        QVector<QVariant> d;
        for (double i = firstPoint; i < firstPoint + interval / data.oneStep; i += period)
            d.push_back(disp[i]);

        d.push_back(noiseLevel);

        QDataStream stream(&additionalData, QIODevice::WriteOnly);
        stream << QVariant(d.toList());
    }
};

typedef QVector<Pulsar>* Pulsars;


#endif // PULSAR_H
