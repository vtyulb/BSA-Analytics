#include <pulsarprocess.h>
#include <math.h>
#include <pulsarworker.h>

#include <QThreadPool>

PulsarProcess::PulsarProcess(QString file, QObject *parent):
    QThread(parent),
    file(file)
{
    Reader reader;
    data = reader.readBinaryFile(file);
}

void PulsarProcess::run() {
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
        for (int j = 0; j < workers[i]->res.size(); j++)
            printf("%s\n", workers[i]->res[j].print().toUtf8().constData());

    qDebug() << data.name << "finished";
}
