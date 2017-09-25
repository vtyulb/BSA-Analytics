#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H

#include <QVector>

#include <data.h>

class CrossCorrelation
{
public:
    QVector<double> process(const Data &data, int module, int ray, int offset);
    Data determinePreciseInterval(const Data &data, int &dispersion);

    static int window();

private:
    QVector<float> profile[32];

    double correlation(int disp);
    void subtractNull(QVector<double> &data);
};

#endif // CROSSCORRELATION_H
