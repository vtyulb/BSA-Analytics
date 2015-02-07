#include "pulsarworker.h"
#include <math.h>
#include <algorithm>
#include <QDebug>
#include <QTimer>
#include <QLinkedList>
#include <QMap>
#include <settings.h>

using std::min;
using std::max;

PulsarWorker::PulsarWorker(int module, int ray, int D, Data data):
    QObject(),
    QRunnable(),
    finished(false),
    data(data),
    module(module),
    ray(ray),
    D(D)
{
}

void PulsarWorker::run() {
    QTime t = QTime::currentTime();
    clearAverange();
    res = searchIn();
    finished = true;
    qDebug() << "process" << module << ray << D << "finished at" << QTime::currentTime().toString() << "total time: " << t.msecsTo(QTime::currentTime()) / 1000.0 << "s , found" << res.size() << "pulsars";
}

QVector<Pulsar> PulsarWorker::searchIn() {
    QVector<Pulsar> pulsars;
    QVector<double> res = applyDispersion();

    QMap<int, double> noises;

    for (double period = MINIMUM_PERIOD / data.oneStep; period < MAXIMUM_PERIOD / data.oneStep; period += data.oneStep / interval)
//        if (!goodDoubles(period, INTERVAL))
    {
        const int duration = interval / data.oneStep / period;
        Pulsar pulsar;
        pulsar.snr = 0;
        int calc = 0;
        double noise = calculateNoise(res.data(), (interval / data.oneStep + 1) * 2);
        for (int i = 0; i < res.size() - interval / data.oneStep; i++) {
            if (calc++ == int(period + 1)) {
                calc = 0;
                i += interval / 2 /data.oneStep;
                if (i < res.size() - interval / data.oneStep) {
                    if (noises.contains(i))
                        noise = noises[i];
                    else {
                        noise = calculateNoise(res.data() + i, min(int(interval / data.oneStep + 1), res.size() - i));
                        noises[i] = noise;
                    }
                }

                if (i >= res.size() - interval / data.oneStep - 1)
                    break;
            }

            double sum = 0;
            double j = i;
            for (int k = 0; k < duration; j += period, k++)
                sum += res[int(j)];

            sum /= duration;
            sum *= sqrt(duration);

            if (sum / noise > pulsar.snr) {
                pulsar.data = data;
                pulsar.module = module;
                pulsar.ray = ray;
                pulsar.firstPoint = i;
                pulsar.period = period * data.oneStep;
                pulsar.dispersion = D;
                pulsar.valid = true;
                pulsar.snr = sum / noise;
                pulsar.name = data.name;
                pulsar.noiseLevel = noise;
                pulsar.filtered = false;
            }
        }

        int good = 0;
        if (pulsar.snr > CATEGORIES_SIZES[0] && Settings::settings()->intellectualFilter()) {
            for (int i = pulsar.firstPoint; i < int(pulsar.firstPoint + period); i++) {
                double sum = 0;
                double j = i;
                for (int k = 0; k < duration; j += period, k++)
                    sum += res[int(j)];

                sum /= duration;
                sum *= sqrt(duration);

                if (sum / noise > 0.5)
                    good++;
            }
        }

        if (pulsar.snr > CATEGORIES_SIZES[0]) {
            if (Settings::settings()->intellectualFilter() && (good > 3))
                pulsar.filtered = true;

            pulsars.push_back(pulsar);
        }
    }

    pulsars = removeDuplicates(pulsars);
    for (int i = 0; i < pulsars.size(); i++)
        pulsars[i].calculateAdditionalData(res);

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
    QLinkedList<Pulsar> l;
    for (int i = 0; i < pulsars.size(); i++)
        l.push_back(pulsars[i]);

    pulsars.clear();

    for (QLinkedList<Pulsar>::Iterator i = l.begin(); i != l.end(); i++)
        for (QLinkedList<Pulsar>::Iterator j = i + 1; j != l.end(); j++)
            if ((*i).valid && (*j).valid)
                equalPulsars(&*i, &*j);

    for (QLinkedList<Pulsar>::Iterator i = l.begin(); i != l.end();)
        if (!(*i).valid)
            i = l.erase(i);
        else
            i++;


    for (QLinkedList<Pulsar>::Iterator i = l.begin(); i != l.end(); i++)
        pulsars.push_back(*i);

    return pulsars;
}

QVector<double> PulsarWorker::applyDispersion() {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    double mxd = (4.1488) * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D;
    mxd *= -data.channels;
    mxd /= data.oneStep;

    QVector<double> res(data.npoints, 0);

    for (int i = 0; i < data.channels - 1; i++) {
        double noise = calculateNoise(data.data[module][i][ray], data.npoints);
        for (int j = 0; j < data.npoints; j++)
            data.data[module][i][ray][j] /= noise;
    }

    for (int i = 0; i < data.npoints - mxd; i++)
        for (int j = 0; j < data.channels - 1; j++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D * j / data.oneStep + 0.5);
            res[i] += data.data[module][j][ray][max(i + dt, 0)];
        }

    for (int i = data.npoints - mxd; i < data.npoints; i++)
        for (int j = 0; j < data.channels - 1; j++)
            res[i] += data.data[module][j][ray][i];

    for (int i = 0; i < res.size(); i++)
        res[i] /= (data.channels - 1);


    double noise = calculateNoise(res.data(), res.size());

    for (int i = 0; i < res.size(); i++)
        if (res[i] >  noise * 4)
            res[i] = noise * 4;
        else if (res[i] < -noise * 4)
            res[i] = -noise * 4;

    return res;
}

void PulsarWorker::clearAverange() {
    const int step =  INTERVAL / data.oneStep;
    for (int channel = 0; channel < data.channels - 1; channel++) {
        for (int i = 0; i < data.npoints; i += step)
            subtract(data.data[module][channel][ray] + i, min(step, data.npoints - i));

        double noise = 0;
        for (int i = 0; i < data.npoints; i++)
            noise += pow(data.data[module][channel][ray][i], 2);

        noise /= data.npoints;
        noise = pow(noise, 0.5);

        const int little = 15;

        for (int i = 0; i < data.npoints - little; i += little) {
            double sum = 0;
            for (int j = i; j < i + little; j++)
                sum += data.data[module][channel][ray][j];

            sum /= little;
            sum = fabs(sum);

            if (sum > noise * 2) {
//                qDebug() << "clearing stair" << i;

                for (int j = i - little * 5; j < i + 60 / data.oneStep && j < data.npoints; j++)
                    data.data[module][channel][ray][j] = (qrand() / double(RAND_MAX) * noise - noise / 2) / 4   ;

                break;
            }
        }
    }
}

template <typename real>
double PulsarWorker::calculateNoise(real *res, int size) {
    QVector<double> noises;

    for (int i = 0; i < size - interval / data.oneStep / 3; i += interval / data.oneStep / 3) {
        double noise = 0;
        for (int j = 0; j < interval / data.oneStep / 3; j++)
            noise += res[i + j] * res[i + j];

        noise /= (interval / data.oneStep / 3);
        noise = pow(noise, 0.5);
        noises.push_back(noise);
    }

    std::sort(noises.begin(), noises.end());
    return noises[noises.size() / 2];
}

template <typename real>
void PulsarWorker::subtract(real *res, int size) {
    double a = 0;
    double b = 0;
    for (int i = 0; i < size / 2; i++)
        a += res[i] / (size / 2);

    for (int i = 1; i <= size / 2; i++)
        b += res[size - i] / (size / 2);

    for (int i = 0; i < size; i++)
        res[i] -= (b - a) * i / size + a;
}
