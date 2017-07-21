#ifndef CROSSCORRELATION_H
#define CROSSCORRELATION_H

#include <QVector>

#include <data.h>

class CrossCorrelation
{
public:
    static QVector<float> process(const Data &data, int module, int ray, int offset);

private:
    static double findBaseline(const Data &data, int module, int ray, int offset);
};

#endif // CROSSCORRELATION_H
