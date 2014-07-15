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

    Data readFile(QString, int skip, int firstColumn); // 0 - true; 1 - false; 2 - autodetect
private:
    int number(QByteArray&);

signals:
    void progress(int);
};

#endif // READER_H
