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
    explicit PulsarWorker(int, int, int, Data, bool);
    ~PulsarWorker() {};

    QVector<Pulsar> res;
    bool finished;
    void run();

private:
    QVector<double> applyDispersion();
    QVector<Pulsar> searchIn();
    void clearAverange();

    QVector<Pulsar> removeDuplicates(QVector<Pulsar>);
    bool equalPulsars(Pulsar*, Pulsar*);
    bool goodDoubles(double, double);

    template <typename real>
    double calculateNoise(real *, int);

    Data data;
    int module, ray, D;
    bool secondInterval;

signals:

public slots:

};

#endif // PULSARWORKER_H
