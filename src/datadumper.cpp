#include "datadumper.h"

#include <QFile>
#include <QDataStream>
#include <QString>

const char *header = "source      source\n"
                     "alpha       alpha\n"
                     "delta       delta\n"
                     "fcentral    110.25\n"
                     "wb_total    2.5\n"
                     "date_begin  23.10.2015 UTC 23.10.2015\n"
                     "time_begin  23:20:47 UTC 19:20:47\n"
                     "date_end    23.10.2015\n"
                     "time_end    21:20:47\n"
                     "modulus      1 2 3 4 5 6\n"
                     "tresolution 99.9424\n"
                     "nbands      6\n"
                     "wbands      0.4150390625 0.4150390625 0.4150390625 0.4150390625 0.4150390625 0.4248046875\n"
                     "fbands      109.20751953125 109.62255859375 110.03759765625 110.45263671875 110.86767578125 111.28759765625\n";

void DataDumper::dump(const Data &data, QFile &f, QMap<QString, QString> headerAddition) {
    f.write("numpar      ");
    f.write(QString::number(16 + headerAddition.size()).toUtf8());
    f.write("\n");

    auto headIt = headerAddition.begin();
    for (int i = 0; i < headerAddition.size(); i++) {
        f.write((headIt.key() + "\t" + headIt.value() + "\n").toUtf8());
        headIt++;
    }

    f.write(header);
    f.write(QString::asprintf("npoints     %d\n", data.npoints).toUtf8());
    for (int i = 0; i < data.npoints; i++)
        for (int m = 0; m < data.modules; m++)
            for (int j = 0; j < data.rays; j++)
                for (int k = 0; k < data.channels; k++)
                    f.write((char*)(void*)&data.data[m][k][j][i], sizeof(float));

    f.close();
}
