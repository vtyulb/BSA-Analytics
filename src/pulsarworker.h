#ifndef PULSARWORKER_H
#define PULSARWORKER_H

#include <QRunnable>
#include <QVector>
#include <data.h>
#include <pulsar.h>

class PulsarWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit PulsarWorker(int, int, int, Data);
    ~PulsarWorker() {};

    QVector<Pulsar> res;
    void run();

private:
    QVector<double> applyDispersion();
    QVector<Pulsar> searchIn();
    void clearAverange();

    QVector<Pulsar> removeDuplicates(QVector<Pulsar>);
    bool equalPulsars(Pulsar*, Pulsar*);
    bool goodDoubles(double, double);

    Data data;
    int module, ray, D;


signals:

public slots:

};

#endif // PULSARWORKER_H
