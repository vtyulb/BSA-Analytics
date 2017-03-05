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
    explicit PulsarWorker(int, int, double, Data, bool sigmaCut = true);
    ~PulsarWorker() {};

    QVector<Pulsar> res;
    bool finished;
    void run();

    template <typename real>
    static void subtract(real *, int);
    template <typename real>
    double calculateNoise(real *, int);

private:
    QVector<double> applyDispersion();
    QVector<Pulsar> searchIn();
    void clearAverange();

    QVector<Pulsar> removeDuplicates(QVector<Pulsar>);
    bool equalPulsars(Pulsar*, Pulsar*);

    bool goodDoubles(double, double);
    void periodTesterWork(QVector<Pulsar> &p);

    Data data;
    int module, ray;
    double D;
    bool sigmaCut;
};

template <typename real>
void PulsarWorker::subtract(real *res, int size) {
    double a = 0;
    double b = 0;
    for (int i = 0; i < size / 2; i++)
        a += res[i] / (size / 2);

    for (int i = 1; i <= size / 2; i++)
        b += res[size - i] / (size / 2);

    double fp = (size / 2 - 1) / 2.0;
    double sp = size - fp - 1;

    double ar = (a - b) * sp / (sp - fp) + b;
    double br = (b - a) * (size - fp) / (sp - fp) + a;
    for (int i = 0; i < size; i++)
        res[i] -= (br - ar) * i / size + ar;
}

template <typename real>
double PulsarWorker::calculateNoise(real *res, int size) {
    QVector<double> noises;

    for (int i = 0; i < size - interval / data.oneStep / 3; i += interval / data.oneStep / 3) {
        double noise = 0;
        for (int j = 0; j < interval / data.oneStep / 3; j++)
            noise += res[i + j] * res[i + j];

        noise /= (interval / data.oneStep / 3);
        noise = pow(noise, 0.5);
        noises.push_back(noise);
    }

    std::sort(noises.begin(), noises.end());
    return noises[noises.size() / 2];
}

#endif // PULSARWORKER_H
