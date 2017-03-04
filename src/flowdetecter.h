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
                          double period, QTime time, QString fileName, bool clearNoise, QObject *parent = 0);
    ~FlowDetecter() {};

    void run();

  private:
    int module;
    int dispersion;
    int ray;
    int points;

    bool trackImpulses;
    int sensitivity;
    bool clearNoise;

    double period;

    QTime time;
    QVector<double> res;
    Data data;
    QString fileName;

    QVector<double> applyDispersion();
};

#endif // FLOWDETECTER_H
