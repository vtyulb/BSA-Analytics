#ifndef DATADUMPER_H
#define DATADUMPER_H

#include <QString>
#include <QFile>

#include <data.h>

class DataDumper
{
public:
    static void dump(const Data &data, QFile &f);

private:
    DataDumper() {};
};

#endif // DATADUMPER_H
