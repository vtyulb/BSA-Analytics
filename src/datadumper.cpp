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
                     "wbands      0.4150390625 0.4150390625 0.4150390625 0.4150390625 0.4150390625 0.4248046875\n";

void DataDumper::dump(const Data &data, QFile &f, QMap<QString, QString> headerAddition) {
    headerAddition["nbands"] = QString::number(data.channels - 1);
    for (int i = 0; i < data.modules; i++)
        headerAddition["modulus"] += " " + QString::number(i);

    headerAddition["tresolution"] = QString::number(data.oneStep * 1000);
    headerAddition["fbands"] = QString::number(data.fbands[0]);
    for (int i = 1; i < data.channels - 1; i++)
        headerAddition["fbands"] += " " + QString::number(data.fbands[i]);

    f.write("numpar      ");
    f.write(QString::number(12 + headerAddition.size()).toUtf8());
    f.write("\n");

    QMap<QString, QString>::Iterator headIt = headerAddition.begin();
    for (int i = 0; i < headerAddition.size(); i++) {
        f.write((headIt.key() + "\t" + headIt.value() + "\n").toUtf8());
        headIt++;
    }

    f.write(header);
    f.write(QString::asprintf("npoints     %d\n", data.npoints).toUtf8());

    QVector<float> res;
    for (int i = 0; i < data.npoints; i++)
        for (int m = 0; m < data.modules; m++)
            for (int j = 0; j < data.rays; j++)
                for (int k = 0; k < data.channels; k++) {
                    res.push_back(data.data[m][k][j][i]);
                    if (res.size() == 1024 * 1024) {
                        f.write((char*)res.data(), res.size() * sizeof(float));
                        res.clear();
                    }
                }

    f.write((char*)res.data(), res.size() * sizeof(float));
    f.close();
}
