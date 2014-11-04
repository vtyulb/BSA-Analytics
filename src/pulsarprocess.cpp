#include "pulsarprocess.h"
#include <math.h>

using std::min;

const int INTERVAL = 5; // in seconds

PulsarProcess::PulsarProcess(QString file, QObject *parent):
    QThread(parent),
    file(file)
{
    Reader reader;
    data = reader.readBinaryFile(file);
}

void PulsarProcess::run() {
    clearAverange();
    QVector<Pulsar> pulsars;
    for (int D = 0; D < 50; D += 6)
        for (int i = 0; i < data.modules; i++)
            for (int j = 0; j < data.rays; j++)
                pulsars += removeDuplicates(searchIn(i, j, D));

    for (int i = 0; i < pulsars.size(); i++)
        qDebug() << pulsars[i].print();
}

QVector<Pulsar> PulsarProcess::searchIn(int module, int ray, int D) {
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
        for (int i = 0; i < res.size() - duration * period; i % int(period * 100 + 0.001) == 0 ? i += 60 /data.oneStep : i++) {
            double sum = 0;
            double j = i;
            for (int k = 0; k < duration; j += period, k++)
                sum += res[int(j)];

            sum /= duration;
            sum *= sqrt(120 / period);

            if (sum > 6 * noise) {
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

bool PulsarProcess::goodDoubles(double a, double b) {
    if (a > b)
        a /= b;
    else
        a = b / a;

    for (int i = 1; i < 100; i++)
        if (fabs(a - i) < 0.05)
            return true;

    return false;
}

bool PulsarProcess::equalPulsars(Pulsar &a, Pulsar &b) {
    if (fabs(a.period - INTERVAL) < 0.02)
        a.valid = false;

    if (fabs(b.period - INTERVAL) < 0.02)
        b.valid = false;

    if (goodDoubles(a.period, b.period) && a.dispersion == b.dispersion && a.ray == b.ray && a.module == b.module) {
        if (a.snr > b.snr)
            b.valid = false;
        else
            a.valid = false;

        return true;
    }

    return false;
}

QVector<Pulsar> PulsarProcess::removeDuplicates(QVector<Pulsar> pulsars) {
    for (int i = 0; i < pulsars.size(); i++)
        for (int j = i + 1; j < pulsars.size(); j++)
            equalPulsars(pulsars[i], pulsars[j]);

    for (int i = 0; i < pulsars.size(); i++)
        if (!pulsars[i].valid) {
            pulsars.remove(i);
            i--;
        }


    for (int i = 0; i < pulsars.size(); i++)
        for (int j = i + 1; j < pulsars.size(); j++)
            if (pulsars[i].snr < pulsars[j].snr) {
                Pulsar p = pulsars[i];
                pulsars[i] = pulsars[j];
                pulsars[j] = p;
            }

    return pulsars;
}

void PulsarProcess::clearAverange() {
    const int step = INTERVAL / data.oneStep;
    for (int channel = 0; channel < data.channels; channel++)
        for (int ray = 0; ray < data.rays; ray++)
            for (int module = 0; module < data.modules; module++) {
                for (int i = 0; i < data.npoints; i += step) {
                    double sum = 0;
                    for (int j = i; j < i + step && j < data.npoints; j++)
                        sum += data.data[module][channel][ray][j];

                    sum /= min(i + step, data.npoints) - i;

                    for (int j = i; j < i + step && j < data.npoints; j++)
                        data.data[module][channel][ray][j] -= sum;
                }

                double noise = 0;
                for (int i = 0; i < data.npoints; i++)
                    noise += pow(data.data[module][channel][ray][i], 2);

                noise /= data.npoints;
                noise = pow(noise, 0.5);

                const int little = 12;

                for (int i = 0; i < data.npoints - little; i += little) {
                    double sum = 0;
                    for (int j = i; j < i + little; j++)
                        sum += data.data[module][channel][ray][j];

                    sum /= little;
                    sum = fabs(sum);

                    if (sum > noise * 3) {
                        for (int j = i - little * 2; j < i + 60 / data.oneStep && j < data.npoints; j++)
                            data.data[module][channel][ray][j] = 0;

                        break;
                    }
                }

                for (int i = 0; i < data.npoints; i++)
                    if (fabs(data.data[module][channel][ray][i]) >  noise * 5) {
                        if (data.data[module][channel][ray][i] > 0)
                            data.data[module][channel][ray][i] = noise * 5;
                        else
                            data.data[module][channel][ray][i] = -noise * 5;
                    }
            }
}

QVector<double> PulsarProcess::applyDispersion(int module, int ray, int D) {
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

    return res;
}
