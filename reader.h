#ifndef READER_H
#define READER_H

#include <QObject>
#include <QDebug>
#include <data.h>

class Reader : public QObject
{
    Q_OBJECT
public:
    explicit Reader(QObject *parent = 0);

    Data readFile(QString);
private:
    int number(QByteArray&);

signals:
    void progress(double);
};

#endif // READER_H
