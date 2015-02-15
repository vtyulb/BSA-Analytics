#ifndef PULSARREADER_H
#define PULSARREADER_H

#include <pulsar.h>
#include <QVector>
#include <QString>
#include <QProgressBar>

class PulsarReader {
    public:
        static Pulsars ReadPulsarFile(QString name, QProgressBar *bar = NULL);
};

#endif // PULSARREADER_H
