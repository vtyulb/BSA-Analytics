#ifndef PRECISEPERIODDETECTER_H
#define PRECISEPERIODDETECTER_H

#include <QString>
#include <QTime>

#include <data.h>

class PrecisePeriodDetecter
{
public:
    PrecisePeriodDetecter() = delete;

    static void detect(QString file1, QString file2, QString file3, int module, int ray, int dispersion, double period, QTime time);

private:
    static double getPhase(Data data, int point);
    static double check(double, double, double, double);
};

#endif // PRECISEPERIODDETECTER_H
