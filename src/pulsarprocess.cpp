#include "pulsarprocess.h"
#include <math.h>

using std::min;

int INTERVAL = 5; // in seconds

PulsarProcess::PulsarProcess(QString file, QObject *parent):
    QThread(parent),
    file(file)
{
    Reader reader;
    data = reader.readBinaryFile(file);
}

void PulsarProcess::run() {
   clearAverange();

   static int D = 0;
   D++;
   QVector<Pulsar> pulsars = removeDuplicates(searchIn(5, 6, 26));
   for (int i = 0; i < pulsars.size(); i++)
       qDebug() << pulsars[i].print();
   qDebug() << "disp" << D;
}

QVector<Pulsar> PulsarProcess::searchIn(int module, int ray, int D) {
    QVector<Pulsar> pulsars;
    QVector<double> res = applyDispersion(module, ray, D); // module 6, ray 7
    for (int i = 0; i < data.npoints; i++)
        data.data[0][0][0][i] = res[i];

    double noise = 0;
    for (int i = 0; i < data.npoints; i++)
        noise += res[i] * res[i];

    noise /= data.npoints;
    noise = pow(noise, 0.5);

    for (double period = 5; period < 100; period += 0.2) {
        const int duration = 120 / data.oneStep / period;
        for (int i = 0; i < res.size() - duration * period; i++) {
            double sum = 0;
            double j = i;
            for (int k = 0; k < duration; j += period, k++)
                sum += res[int(j)];

            sum /= duration;

            if (sum > noise) {
                Pulsar pulsar;
                pulsar.data = data;
                pulsar.module = module;
                pulsar.ray = ray;
                pulsar.firstPoint = i;
                pulsar.period = period;
                pulsar.dispersion = D;
                pulsar.valid = true;
                pulsars.push_back(pulsar);
            }
        }
    }

    return  pulsars;
}

bool PulsarProcess::equalPulsars(Pulsar a, Pulsar b) {
    if (abs(a.firstPoint - b.firstPoint) < 60 / data.oneStep) {
        if (a.period > b.period)
            a.valid = false;
        else if (a.period < b.period)
            b.valid = false;
        else if (a.firstPoint < b.firstPoint)
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
            if (equalPulsars(pulsars[i], pulsars[j])) {
                if (pulsars[i].valid)
                    pulsars.remove(j);
                else
                    pulsars.remove(i);

                i--;
                break;
            }

    return pulsars;
}

void PulsarProcess::clearAverange() {
    const int step = INTERVAL / data.oneStep;
    for (int channel = 0; channel < data.channels; channel++)
        for (int ray = 0; ray < data.rays; ray++)
            for (int module = 0; module < data.modules; module++)
                for (int i = 0; i < data.npoints; i += step) {
                    double sum = 0;
                    for (int j = i; j < i + step && j < data.npoints; j++)
                        sum += data.data[module][channel][ray][j];

                    sum /= min(i + step, data.npoints) - i;

                    for (int j = i; j < i + step && j < data.npoints; j++)
                        data.data[module][channel][ray][j] -= sum;
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
