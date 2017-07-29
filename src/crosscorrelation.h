#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H

#include <QVector>

#include <data.h>

const int CROSS_CORRELATION_SIZE = 1024;
const int CROSS_CORRELATION_WINDOW = 3;
const int CROSS_CORRELATION_FIRST_DISPERSION = 0;
const int CROSS_CORRELATION_LAST_DISPERSION = 550;

class CrossCorrelation
{
public:
    QVector<double> process(const Data &data, int module, int ray, int offset);
    Data determinePreciseInterval(const Data &data, int &dispersion);

private:
    QVector<float> profile[32];

    double correlation(int disp);
    void subtractNull(QVector<double> &data);
};

#endif // CROSSCORRELATION_H
