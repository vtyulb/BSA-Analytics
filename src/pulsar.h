#ifndef PULSAR_H
#define PULSAR_H

#include <data.h>
#include <startime.h>
#include <math.h>
#include <QString>
#include <QVector>
#include <QVariant>

const double INTERVAL = 17;
const double MINIMUM_PERIOD = 0.5;
const double MAXIMUM_PERIOD = 10;
const double PERIOD_STEP = 0.01;

const int interval = 180;
const int CATEGORIES = 4;
const int CATEGORIES_SIZES[CATEGORIES + 1] = {4, 5, 7, 10, 1000};

struct Pulsar {
    Data data;
    int module;
    int ray;
    int dispersion;
    int firstPoint;
    double period; // in seconds;
    double snr;
    bool filtered;

    bool badNoiseKnown = false;
    bool badNoiseRes;

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

    QString time() const {
        return StarTime::StarTime(data, firstPoint + interval / 2 / data.oneStep);
    }

    void calculateAdditionalData(const QVector<double> &disp) {
        QVector<QVariant> d;
        for (int offset = -period / data.oneStep / 2; offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1; offset++) {
            double sum = 0;
            int n = 0;
            for (double i = firstPoint + offset; i < firstPoint + offset + interval / data.oneStep; i += period / data.oneStep * 2, n++)
                sum += disp[int(i)];

            d.push_back(sum / n);
        }

        for (int i = 0; i < 100; i++)
            d.push_back(0);

        for (int i = firstPoint; i < firstPoint + interval / data.oneStep; i++)
            d.push_back(disp[i]);


        QDataStream stream(&additionalData, QIODevice::WriteOnly);
        stream << QVariant(d.toList());
    }

    void squeeze() {
        int i = 0;
        while (data.data[0][0][0][i] != 0)
            i++;

        data.npoints = i + 10;
        float *nd = new float[i + 10];
        memcpy(nd, data.data[0][0][0], sizeof(float) * (i + 10));

        delete[] data.data[0][0][0];
        data.data[0][0][0] = nd;
    }

    bool operator < (const Pulsar &p) const {        
        if (nativeTime.secsTo(p.nativeTime) > 0)
            return true;
        else if (nativeTime.secsTo(p.nativeTime) < 0)
            return false;

        if (dispersion < p.dispersion)
            return true;

        return false;
    }

    static bool secondComparator(const Pulsar &p1, const Pulsar &p2) {
        if (!p1.filtered && p2.filtered)
            return true;
        else if (p1.filtered && !p2.filtered)
            return false;

        return p1 < p2;
    }

    bool badNoise() const {
        if (badNoiseKnown)
            return badNoiseRes;

        int j = 0;
        while (data.data[0][0][0][j] != 0) j++;
        while (data.data[0][0][0][j] == 0) j++;

        QVector<double> sigmas;
        int n = data.npoints - j;
        float *dt = data.data[0][0][0] + j;
        bool res = true;
        const int pieces = 8;
        for (int k = 0; k < pieces; k++) {
            double sigma = 0;
            for (int i = k * n / pieces; i < (k + 1) * n / pieces; i++)
                sigma += dt[i] * dt[i];

            sigma /= (n / pieces);
            sigma = pow(sigma, 0.5);
            sigmas.push_back(sigma);
        }

        for (int i = 0; i < pieces; i++)
            for (int j = 0; j < pieces; j++)
                if (sigmas[i] / sigmas[j] > 3)
                    res = false;

        return !res;
    }
};

typedef QVector<Pulsar>* Pulsars;


#endif // PULSAR_H
