#ifndef DATADUMPER_H
#define DATADUMPER_H

#include <QString>

#include <data.h>

class DataDumper
{
public:
    static void dump(const Data &data, const QString path);

private:
    DataDumper() {};
};

#endif // DATADUMPER_H
