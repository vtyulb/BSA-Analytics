#include <pulsarprocess.h>
#include <math.h>
#include <pulsarworker.h>
#include <calculationpool.h>
#include <settings.h>

#include <QThreadPool>
#include <QFile>
#include <QDir>

using std::max;

PulsarProcess::PulsarProcess(QString file, QString savePath, QObject *parent):
    QThread(parent),
    savePath(savePath),
    file(file)
{
    Reader reader;
    data = reader.readBinaryFile(file);

    if (Settings::settings()->preciseSearch())
        data.halfRelease(Settings::settings()->module(), Settings::settings()->ray());
}

PulsarProcess::~PulsarProcess() {
//    Yes, I know, that it is memory leak. Nobody cares!
//    data.releaseData();
//    qDebug() << "data released";
}


void PulsarProcess::run() {
    QVector<PulsarWorker*> workers;
    QThreadPool *pool = CalculationPool::pool();
    qDebug() << "splitting to" << pool->maxThreadCount() << "processes";

    if (!Settings::settings()->preciseSearch()) {
        for (int D = 0; D < 200; D += 6)
            for (int i = 0; i < data.modules; i++)
                for (int j = 0; j < data.rays; j++) {
                        workers.push_back(new PulsarWorker(i, j, D, data));
                        workers[workers.size() - 1]->setAutoDelete(false);
                        pool->start(workers[workers.size() - 1]);
                    }
    } else if (Settings::settings()->periodTester()) {
        workers.push_back(new PulsarWorker(Settings::settings()->module(), Settings::settings()->ray(), Settings::settings()->dispersion(), data));
        workers[0]->setAutoDelete(false);
        pool->start(workers[0]);
    } else {
        int mx = 200;
        if (Settings::settings()->dispersion() > -0.5)
            mx = Settings::settings()->dispersion() + 6;

        int step = 1;
        if (data.oneStep > 0.05)
            step = 3;

        for (double D = max(Settings::settings()->dispersion() - 6, 0.0); D < mx; D += step) {
            workers.push_back(new PulsarWorker(Settings::settings()->module(), Settings::settings()->ray(), D, data));
            workers[workers.size() - 1]->setAutoDelete(false);
            pool->start(workers[workers.size() - 1]);
        }

    }

    while (!pool->waitForDone(30000)) {
        bool finished = true;
        for (int i = 0; i < workers.size(); i++)
            finished &= workers[i]->finished;

        if (finished)
            break;
    }

    QVector<Pulsar> pulsars;
    for (int i = 0; i < workers.size(); i++) {
        for (int j = 0; j < workers[i]->res.size(); j++)
            pulsars.push_back(workers[i]->res[j]);

//        workers[i]->deleteLater();
    }

    qDebug() << "workers deleted";

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = i + 1; j < pulsars.size(); j++)
            if ((pulsars[i].filtered && !pulsars[j].filtered) || ((pulsars[i].filtered == pulsars[j].filtered)
                                                                  && (pulsars[i].firstPoint > pulsars[j].firstPoint))) {
                Pulsar p = pulsars[i];
                pulsars[i] = pulsars[j];
                pulsars[j] = p;
            }

    QByteArray header = QString("file: %1\ntresolution %2\nStart time\tmodule\tray\tdispersion\tperiod\tsnr\tfirst_point\n").arg(data.name).arg(data.oneStep).toUtf8();

    QFile *files[CATEGORIES];
    bool filtered[CATEGORIES];
    for (int i = 0; i < CATEGORIES; i++)
        filtered[i] = false;

    QDir().mkdir(savePath);
    for (int i = 0; i < CATEGORIES; i++) {
        files[i] = new QFile(savePath + QString("%1-%2-%3.pulsar").arg(data.name).arg(CATEGORIES_SIZES[i]).arg(CATEGORIES_SIZES[i + 1]));
        files[i]->open(QIODevice::WriteOnly);
        files[i]->write(header);
    }

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = CATEGORIES - 1; j >= 0; j--)
            if (CATEGORIES_SIZES[j] < pulsars[i].snr || j == 0) {
                if (!filtered[j] && pulsars[i].filtered) {
                    filtered[j] = true;
                    files[j]->write(QByteArray("filtered next\n"));
                }

                QByteArray d = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\n").
                        arg(pulsars[i].time()).
                        arg(pulsars[i].module + 1).
                        arg(pulsars[i].ray + 1).
                        arg(pulsars[i].dispersion).
                        arg(QString::number(pulsars[i].period, 'f', 5)).
                        arg(QString::number(pulsars[i].snr, 'f', 1)).
                        arg(QString::number(pulsars[i].firstPoint)).
                        toUtf8();

                files[j]->write(d);
                break;
            }

    for (int i = 0; i < CATEGORIES; i++)
        files[i]->write("additional data:\n");

    for (int i = 0; i < pulsars.size(); i++)
        for (int j = CATEGORIES - 1; j >= 0; j--)
            if (CATEGORIES_SIZES[j] < pulsars[i].snr || j == 0) {
                files[j]->write(pulsars[i].additionalData);
                break;
            }

    for (int i = 0; i < CATEGORIES; i++) {
        files[i]->close();
        delete files[i];
    }

    qDebug() << "files deleted";
}
