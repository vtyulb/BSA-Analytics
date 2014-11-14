#ifndef PULSARREADER_H
#define PULSARREADER_H

#include <pulsar.h>
#include <QVector>
#include <QString>

class PulsarReader {
    public:
        static QVector<Pulsar> ReadPulsarFile(QString name);
};

#endif // PULSARREADER_H
