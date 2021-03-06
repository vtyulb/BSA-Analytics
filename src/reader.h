#ifndef READER_H
#define READER_H

#include <QObject>
#include <QDebug>
#include <QMap>
#include <data.h>

class Reader : public QObject
{
    Q_OBJECT
public:
    explicit Reader(QObject *parent = 0);

    Data readFile(QString, int skip, int firstColumn, QDateTime = QDateTime(), bool binary = false); // 0 - true; 1 - false; 2 - autodetect
    Data readBinaryFile(QString, bool readOnlyHeader = false);

    static void repairWrongChannels(Data &data);

private:
    int number(QByteArray);
    float slowNumber(QByteArray);

signals:
    void progress(int);
};

#endif // READER_H
