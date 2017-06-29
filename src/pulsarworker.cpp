#include "pulsarworker.h"
#include <math.h>
#include <algorithm>
#include <QDebug>
#include <QTimer>
#include <QLinkedList>
#include <QMap>
#include <settings.h>
#include <analytics.h>

using std::min;
using std::max;

PulsarWorker::PulsarWorker(int module, int ray, double D, Data data, bool sigmaCut):
    QObject(),
    QRunnable(),
    finished(false),
    data(data),
    module(module),
    ray(ray),
    D(D),
    sigmaCut(sigmaCut)
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

    int start = 0;
    int end = res.size() - interval / data.oneStep;
    if (Settings::settings()->preciseSearch()) {
        while (abs(Settings::settings()->getTime().secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > 180)
            start++;

        Settings::settings()->getTime().secsTo(QTime::fromString(StarTime::StarTime(data, start)));

        end = start;
        while (abs(Settings::settings()->getTime().secsTo(QTime::fromString(StarTime::StarTime(data, end)))) <= 180)
            end++;

        Settings::settings()->getTime().secsTo(QTime::fromString(StarTime::StarTime(data, end)));

        if (Settings::settings()->singlePeriod())
            end = start + 1 + Settings::settings()->period() / data.oneStep;
    }


    double MINIMUM_PERIOD_INC = MINIMUM_PERIOD;
    double MAXIMUM_PERIOD_INC = MAXIMUM_PERIOD;

    if (Settings::settings()->preciseSearch()) {
        MINIMUM_PERIOD_INC /= 1000;
        MAXIMUM_PERIOD_INC *= 2;
    }

    if (Settings::settings()->singlePeriod()) {
        MINIMUM_PERIOD_INC = Settings::settings()->period();
        MAXIMUM_PERIOD_INC = MINIMUM_PERIOD_INC + 0.000000001;
    }

    int periodTester = 0;
    if (Settings::settings()->periodTester()) {
        qDebug() << "sigma" <<  calculateNoise(res.data(), (interval / data.oneStep + 1) * 2);
        periodTester = 1;
    }

    double noise = calculateNoise(res.data(), (interval / data.oneStep + 1) * 2);
    for (double period = MINIMUM_PERIOD_INC / data.oneStep; period < MAXIMUM_PERIOD_INC / data.oneStep; period += data.oneStep / interval)
        if (!Settings::settings()->preciseSearch() || (goodDoubles(period * data.oneStep, Settings::settings()->period()) &&
                                                       (!Settings::settings()->noMultiplePeriods())) ||
                globalGoodDoubles(period * data.oneStep, Settings::settings()->period()))
    {
        if (periodTester)
            qDebug() << "point" << periodTester++ << " period " << period;

        const int duration = interval / data.oneStep / period;
        Pulsar pulsar;
        pulsar.snr = pulsar.module = pulsar.ray = 0;
        pulsar.data = data;
        int calc = 0;
        if (!Settings::settings()->preciseSearch())
            noise = calculateNoise(res.data(), (interval / data.oneStep + 1) * 2);

        for (int i = start; i < end; i++) {
            if (calc++ == int(period + 1)) {
                calc = 0;
                if (!Settings::settings()->preciseSearch())
                    i += interval / 2 /data.oneStep;
                else
                    i += 20 / data.oneStep;

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

        if (pulsar.snr > CATEGORIES_SIZES[0] || Settings::settings()->preciseSearch()) {
            if (Settings::settings()->intellectualFilter() && (good > 3))
                pulsar.filtered = true;

            if (Settings::settings()->preciseSearch() && !Settings::settings()->periodTester()) {
                if (!pulsars.size())
                    pulsars.push_back(pulsar);
                else if (pulsar.snr > pulsars[0].snr) {
                    pulsars.clear();
                    pulsars.push_back(pulsar);
                }
            } else
                pulsars.push_back(pulsar);
        }
    }

    if (Settings::settings()->periodTester()) {
        for (int i = 0; i < pulsars.size(); i++)
            pulsars[i].calculateAdditionalData(res);

        periodTesterWork(pulsars);
    } else {
        pulsars = removeDuplicates(pulsars);
        for (int i = 0; i < pulsars.size(); i++)
            if (pulsars[i].period < 0.00001) {
                pulsars.removeAt(i);
                i--;
            } else pulsars[i].calculateAdditionalData(res);
    }

    return  pulsars;
}

bool PulsarWorker::goodDoubles(double a, double b) {
    if (a > b)
        a /= b;
    else
        a = b / a;

    const double eps = Settings::settings()->preciseSearch() ? 0.01 : 0.05;
    if (fabs(a - int(a + 0.5)) < eps)
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
            if (equalPulsars(&*i, &*j)) {
                if (!(*i).valid) {
                    i = l.erase(i);
                    j = i;
                } else {
                    j = l.erase(j);
                    j--;
                }
            }

    /*for (QLinkedList<Pulsar>::Iterator i = l.begin(); i != l.end();)
        if (!(*i).valid)
            i = l.erase(i);
        else
            i++;*/


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


    if (sigmaCut && !Settings::settings()->doNotClearNoise()) {
        for (int i = 0; i < res.size(); i++)
            if (res[i] >  noise * 4)
                res[i] = noise * 4;
            else if (res[i] < -noise * 4)
                res[i] = -noise * 4;
    }

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

                for (int j = max(i - little * 5, 0); j < i + 60 / data.oneStep && j < data.npoints; j++)
                    data.data[module][channel][ray][j] = (qrand() / double(RAND_MAX) * noise - noise / 2) / 4   ;

                break;
            }
        }
    }
}

void PulsarWorker::periodTesterWork(QVector<Pulsar> &p) {
    QVector<QVariant> res;
    for (int i = 0; i < p.size(); i++) {
        double min = +1e+10;
        double max = -1e+10;
        QDataStream stream(&p[i].additionalData, QIODevice::ReadOnly);
        QVariant vrt;
        QList<QVariant> data;
        stream >> vrt;
        data = vrt.toList();
        for (int j = 0; data[j].toDouble() != 0; j++) {
            double v = data[j].toDouble();
            if (min > v)
                min = v;
            if (max < v)
                max = v;
        }

        res.push_back(max - min);
    }

    res.push_back(0);
    res.push_back(0);
    res.push_back(0);


    p[0].additionalData = QByteArray();
    QDataStream stream(&p[0].additionalData, QIODevice::WriteOnly);
    stream << QVariant(res.toList());

    p.resize(1);
}
