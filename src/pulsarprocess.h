#ifndef PULSARPROCESS_H
#define PULSARPROCESS_H

#include <QThread>
#include <QString>
#include <data.h>
#include <reader.h>
#include <pulsar.h>

class PulsarProcess : public QThread
{
    Q_OBJECT
public:
    explicit PulsarProcess(QString file, QObject *parent = 0);

    QString file;
    Data data;

private:
    void run();

signals:

public slots:

};

#endif // PULSARPROCESS_H
