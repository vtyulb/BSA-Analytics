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

    Data readFile(QString, int skip, int firstColumn, bool binary = false); // 0 - true; 1 - false; 2 - autodetect
private:
    int number(QByteArray);

    Data readBinaryFile(QString);

signals:
    void progress(int);
};

#endif // READER_H
