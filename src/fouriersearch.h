#ifndef FOURIERSEARCH_H
#define FOURIERSEARCH_H

#include <QString>
#include <QVector>

#include <data.h>

class FourierSearch
{
public:
    static void run(QString pathToCuttedData);

private:
    static void runFourier(const Data &data, QVector<float> &res, int module, int ray, int channel);

    FourierSearch() {};
};

#endif // FOURIERSEARCH_H
