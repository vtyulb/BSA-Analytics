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
    explicit PulsarProcess(QString file, QString savePath, QObject *parent = 0);
    ~PulsarProcess();

    Data data;

private:
    void run();

    QString savePath;
    QString file;

signals:

public slots:

};

#endif // PULSARPROCESS_H
