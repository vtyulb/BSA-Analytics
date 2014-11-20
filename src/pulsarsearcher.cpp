#include <pulsarsearcher.h>
#include <pulsarprocess.h>
#include <calculationpool.h>
#include <settings.h>
#include <QDir>

PulsarSearcher::PulsarSearcher(QString dir, QString savePath, int threads, QObject *parent) :
    QObject(parent),
    savePath(savePath)
{
    files = QDir(dir).entryInfoList();
    if (threads != -1)
        CalculationPool::pool()->setMaxThreadCount(threads);
}

void PulsarSearcher::start() {
    for (int i = 0; i < files.size(); i++)
        if (files[i].isFile())
            if (Settings::settings()->skipCount() == 0) {
                qDebug() << "searching on file" << files[i].absoluteFilePath();
                PulsarProcess *p = new PulsarProcess(files[i].absoluteFilePath(), savePath + files[i].baseName() + "/");
                workers.push_back(p);
                QObject::connect(p, SIGNAL(finished()), this, SLOT(checkIfCalculated()));
                p->start();
            }
}

void PulsarSearcher::checkIfCalculated() {
    qDebug() << "process finished";
    for (int i = 0; i < workers.size(); i++)
        if (workers[i]->isFinished()) {
            delete workers[i];
            workers.remove(i);
            i--;
        }

    qDebug() << "process cleared";

    if (workers.size() == 0)
        exit(0);
}
