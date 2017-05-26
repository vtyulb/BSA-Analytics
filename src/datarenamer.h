#ifndef DATARENAMER_H
#define DATARENAMER_H

#include <QString>

class DataRenamer
{
public:
    DataRenamer() {};

    void run(QString path);
    void repairDir(QString path);
};

#endif // DATARENAMER_H
