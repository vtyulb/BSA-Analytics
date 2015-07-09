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

    static double phase1, phase2, phase3;

private:
    static double getPhase(Data data, int point);
    static double check(double);

};

#endif // PRECISEPERIODDETECTER_H
