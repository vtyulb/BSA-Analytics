#ifndef PRECISEPRERIODDETECTER_H
#define PRECISEPRERIODDETECTER_H

#include <QString>
#include <QTime>

class PrecisePreriodDetecter
{
public:
    PrecisePreriodDetecter() = delete;

    static void detect(QString file1, QString file2, QString file3, int module, int ray, int dispersion, double period, QTime time);
};

#endif // PRECISEPRERIODDETECTER_H
