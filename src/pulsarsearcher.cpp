#include <pulsarsearcher.h>
#include <pulsarprocess.h>
#include <QDir>

PulsarSearcher::PulsarSearcher(QString dir, QObject *parent) :
    QObject(parent)
{
    files = QDir(dir).entryInfoList();
}

void PulsarSearcher::start() {
    for (int i = 0; i < files.size(); i++)
        if (files[i].isFile()) {
            qDebug() << "searching on file" << files[i].absoluteFilePath();
            PulsarProcess *p = new PulsarProcess(files[i].absoluteFilePath());
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
