#include <pulsarsearcher.h>
#include <pulsarprocess.h>
#include <calculationpool.h>
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
        if (files[i].isFile()) {
            qDebug() << "searching on file" << files[i].absoluteFilePath();
            QString relativeSavePath = savePath + files[i].baseName() + "/";
            QDir().mkdir(relativeSavePath);
            PulsarProcess *p = new PulsarProcess(files[i].absoluteFilePath(), relativeSavePath);
            workers.push_back(p);
            QObject::connect(p, SIGNAL(finished()), this, SLOT(checkIfCalculated()));
            p->start();
        }
}

void PulsarSearcher::checkIfCalculated() {
    for (int i = 0; i < workers.size(); i++)
        if (workers[i]->isFinished()) {
            delete workers[i];
            workers.remove(i);
            i--;
        }

    if (workers.size() == 0)
        exit(0);
}
