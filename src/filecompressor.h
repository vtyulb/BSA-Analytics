#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H

#include <QObject>
#include <QString>
#include <pulsar.h>

class FileCompressor
{
public:
    static void compress(QString);

private:
    static void dump(Pulsars, QString);
    static bool equal(float *, float *);
    static int rewind(float *);
};

#endif // FILECOMPRESSOR_H
