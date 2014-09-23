#include <pulsarsearcher.h>
#include <pulsarprocess.h>
#include <QDir>

PulsarSearcher::PulsarSearcher(QString dir, QObject *parent) :
    QObject(parent)
{
    files = QDir(dir).entryInfoList();
}

void PulsarSearcher::start() {
    for (int i = 0; i < files.size(); i++) {
        PulsarProcess *p = new PulsarProcess(files[i].absoluteFilePath());
        p->start();
    }
}
