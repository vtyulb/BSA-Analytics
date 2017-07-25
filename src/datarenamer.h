#ifndef DATARENAMER_H
#define DATARENAMER_H

#include <QString>

class DataRenamer
{
public:
    DataRenamer() {};

    void run(QString path);
    void repairDir(QStringList paths, int block);
};

#endif // DATARENAMER_H
