#include "pulsarworker.h"
#include <math.h>
#include <QDebug>
#include <QTimer>

PulsarWorker::PulsarWorker(int module, int ray, int D, Data data):
    QObject(),
    QRunnable(),
    data(data),
    module(module),
    ray(ray),
    D(D)
{
}

void PulsarWorker::run() {
    QTime t = QTime::currentTime();
    res = removeDuplicates(searchIn(module, ray, D));
    qDebug() << "process" << module << ray << D << "finished at" << QTime::currentTime().toString() << "total time: " << t.secsTo(QTime::currentTime()) << "s , found" << res.size() << "pulsars";
}

QVector<Pulsar> PulsarWorker::searchIn(int module, int ray, int D) {
    QVector<Pulsar> pulsars;
    QVector<double> res = applyDispersion(module, ray, D); // module 6, ray 7
//    for (int i = 0; i < data.npoints; i++)
//        data.data[0][0][0][i] = res[i];

    double noise = 0;
    for (int i = 0; i < data.npoints; i++)
        noise += res[i] * res[i];

    noise /= data.npoints;
    noise = pow(noise, 0.5);

    for (double period = 5; period < 100; period += 0.01) {
//        if (int(period) == int(period + 0.999))
//            printf("calculation period %.2g\n", period);

        const int duration = 120 / data.oneStep / period;
        for (int i = 0; i < res.size() - duration * period; i % int(period * 10 + 0.001) == 0 ? i += 60 /data.oneStep : i++) {
            double sum = 0;
            double j = i;
            for (int k = 0; k < duration; j += period, k++)
                sum += res[int(j)];

            sum /= duration;
            sum *= sqrt(120 / period);

            if (sum > 4 * noise) {
                Pulsar pulsar;
                pulsar.data = data;
                pulsar.module = module;
                pulsar.ray = ray;
                pulsar.firstPoint = i;
                pulsar.period = period * data.oneStep;
                pulsar.dispersion = D;
                pulsar.valid = true;
                pulsar.snr = sum / noise;
                pulsar.name = data.name;
                pulsars.push_back(pulsar);
            }
        }
    }

    return  pulsars;
}

bool PulsarWorker::goodDoubles(double a, double b) {
    if (a > b)
        a /= b;
    else
        a = b / a;

    if (fabs(a - int(a + 0.5)) < 0.05)
        return true;

    return false;
}

bool PulsarWorker::equalPulsars(Pulsar *a, Pulsar *b) {
    if (goodDoubles(a->period, b->period)) {
        if (a->snr > b->snr)
            b->valid = false;
        else
            a->valid = false;

        return true;
    }

    return false;
}

QVector<Pulsar> PulsarWorker::removeDuplicates(QVector<Pulsar> pulsars) {
//    qDebug() << "removing duplicates. Total found" << pulsars.size();
    QList<Pulsar> l;
    for (int i = 0; i < pulsars.size(); i++)
        l.push_back(pulsars[i]);

    pulsars.clear();

    for (QList<Pulsar>::Iterator i = l.begin(); i != l.end();)
        if (goodDoubles(5.0, (*i).period))
            i = l.erase(i);
        else
            i++;

    for (QList<Pulsar>::Iterator i = l.begin(); i != l.end(); i++)
        for (QList<Pulsar>::Iterator j = i + 1; j != l.end(); j++)
            if ((*i).valid && (*j).valid)
                equalPulsars(&*i, &*j);

    for (QList<Pulsar>::Iterator i = l.begin(); i != l.end();)
        if (!(*i).valid)
            i = l.erase(i);
        else
            i++;


    for (QList<Pulsar>::Iterator i = l.begin(); i != l.end(); i++)
        pulsars.push_back(*i);

//    qDebug() << "removed. Total:" << pulsars.size();

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = i + 1; j < pulsars.size(); j++)
            if (pulsars[i].snr < pulsars[j].snr) {
                Pulsar p = pulsars[i];
                pulsars[i] = pulsars[j];
                pulsars[j] = p;
            }

    return pulsars;
}

QVector<double> PulsarWorker::applyDispersion(int module, int ray, int D) {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    double mxd = 4.15 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D;
    mxd *= -data.channels;
    mxd /= data.oneStep;

    QVector<double> res(data.npoints, 0);

    for (int i = 0; i < data.npoints - mxd; i++)
        for (int j = 0; j < data.channels - 1; j++) {
            int dt = 4.15 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D * j / data.oneStep; // difference
            res[i] += data.data[module][j][ray][i - dt];
        }

    for (int i = data.npoints - mxd; i < data.npoints; i++)
        for (int j = 0; j < data.channels - 1; j++)
            res[i] += data.data[module][j][ray][i];

    for (int i = 0; i < res.size(); i++)
        res[i] /= (data.channels - 1);

    return res;
}
