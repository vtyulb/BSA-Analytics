#include <pulsarprocess.h>
#include <math.h>
#include <pulsarworker.h>
#include <calculationpool.h>

#include <QThreadPool>
#include <QFile>

PulsarProcess::PulsarProcess(QString file, QString savePath, QObject *parent):
    QThread(parent),
    file(file),
    savePath(savePath)
{
    Reader reader;
    data = reader.readBinaryFile(file);
}

void PulsarProcess::run() {
    QVector<PulsarWorker*> workers;
    QThreadPool *pool = CalculationPool::pool();
    qDebug() << "splitting to" << pool->maxThreadCount() << "processes";
    for (int D = 0; D < 200; D += 6)
        for (int i = 0; i < data.modules; i++)
            for (int j = 0; j < data.rays; j++) {
                workers.push_back(new PulsarWorker(i, j, D, data));
                workers[workers.size() - 1]->setAutoDelete(false);
                pool->start(workers[workers.size() - 1]);
            }

    pool->waitForDone();
    QVector<Pulsar> pulsars;
    for (int i = 0; i < workers.size(); i++)
        for (int j = 0; j < workers[i]->res.size(); j++)
            pulsars.push_back(workers[i]->res[j]);

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = i + 1; j < pulsars.size(); j++)
            if (pulsars[i].firstPoint > pulsars[j].firstPoint) {
                Pulsar p = pulsars[i];
                pulsars[i] = pulsars[j];
                pulsars[j] = p;
            }

    const int categories = 3;
    const int sz[categories + 1] = {5, 7, 10, 1000};
    QByteArray header = QString("file: %1\nStart time\tmodule\tray\tdispersion\tsnr\n").arg(data.name).toUtf8();

    QFile *files[categories];

    for (int i = 0; i < categories; i++) {
        files[i] = new QFile(savePath + QString("%1-%2-%3.pulsar").arg(data.name).arg(sz[i]).arg(sz[i + 1]));
        files[i]->open(QIODevice::WriteOnly);
        files[i]->write(header);
    }

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = categories - 1; j >= 0; j--)
            if (sz[j] < pulsars[i].snr) {
                QByteArray d = QString("%1\t%2\t%3\t%4\t%5\n").
                        arg(pulsars[i].time()).
                        arg(pulsars[i].module).
                        arg(pulsars[i].ray).
                        arg(pulsars[i].dispersion).
                        arg(pulsars[i].snr).toUtf8();

                files[j]->write(d);
                break;
            }

    for (int i = 0; i < categories; i++)
        files[i]->write("additional data:\n");

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = categories - 1; j >= 0; j--)
            if (sz[j] < pulsars[i].snr) {
                files[j]->write(pulsars[i].additionalData);
                files[j]->write("\n");
                break;
            }

    for (int i = 0; i < categories; i++)
        files[i]->close();
}
