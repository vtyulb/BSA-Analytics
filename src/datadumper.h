#ifndef DATADUMPER_H
#define DATADUMPER_H

#include <QString>
#include <QFile>
#include <QMap>

#include <data.h>

class DataDumper
{
public:
    static void dump(const Data &data, QFile &f, QMap<QString, QString> headerAddition = QMap<QString, QString>());

private:
    DataDumper() {};
};

#endif // DATADUMPER_H
