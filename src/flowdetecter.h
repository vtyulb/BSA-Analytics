#ifndef FLOWDETECTER_H
#define FLOWDETECTER_H

#include <QObject>
#include <QTime>
#include <QVector>

#include <data.h>

class FlowDetecter: public QObject
{
    Q_OBJECT

public:
    explicit FlowDetecter(int module, int dispersion, int ray, int points, bool trackImpulses, int sensitivity,
                          double period, QTime time, QString fileName, QObject *parent = 0);
    ~FlowDetecter() {};

    void run();

private:
    int module;
    int dispersion;
    int ray;
    int points;

    bool trackImpulses;
    int sensitivity;

    double period;

    QTime time;
    QVector<double> res;
    Data data;
    QString fileName;

    double calculateNoise(const QVector<double> &r);
    QVector<double> applyDispersion();
    void showProfile(const QVector<double>&);
};

#endif // FLOWDETECTER_H
