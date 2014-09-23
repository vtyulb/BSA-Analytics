#ifndef PULSARPROCESS_H
#define PULSARPROCESS_H

#include <QThread>
#include <QString>
#include <data.h>
#include <reader.h>

class PulsarProcess : public QThread
{
    Q_OBJECT
public:
    explicit PulsarProcess(QString file, QObject *parent = 0);

private:
    QString file;
    Data data;

    double noise[33][60];
    void run();

signals:

public slots:

};

#endif // PULSARPROCESS_H
