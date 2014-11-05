#include <pulsarprocess.h>
#include <math.h>
#include <pulsarworker.h>

#include <QThreadPool>

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
    QThreadPool pool;
    QVector<PulsarWorker*> workers;
    QVector<Pulsar> pulsars;
    qDebug() << "splitting to" << pool.maxThreadCount() << "processes";
    for (int D = 0; D < 80; D += 6)
        for (int i = 0; i < data.modules; i++)
            for (int j = 0; j < data.rays; j++) {
                workers.push_back(new PulsarWorker(i, j, D, data));
                workers[workers.size() - 1]->setAutoDelete(false);
                pool.start(workers[workers.size() - 1]);
            }

    pool.waitForDone();
    for (int i = 0; i < workers.size(); i++)
        for (int j = 0; j < workers[i]->res.size(); i++)
            qDebug() << workers[i]->res[i].print();

    qDebug() << data.name << "finished";
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
