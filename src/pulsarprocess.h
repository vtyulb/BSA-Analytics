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
    void clearAverange();
    QVector<double> applyDispersion(int module, int ray, int D);
    QVector<Pulsar> searchIn(int module, int ray, int D);

    QVector<Pulsar> removeDuplicates(QVector<Pulsar>);
    bool equalPulsars(Pulsar, Pulsar);

signals:

public slots:

};

#endif // PULSARPROCESS_H
