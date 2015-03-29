#include "filecompressor.h"

#include <QFileInfoList>
#include <QDir>
#include <pulsarreader.h>
#include <algorithm>

void FileCompressor::compress(QString name) {
    QFileInfoList list = QDir(name).entryInfoList(QDir::Dirs);
    for (int i = 0; i < list.size(); i++)
        if (list[i].fileName() != "." && list[i].fileName() != "..")
            compress(list[i].absolutePath());

    list = QDir(name).entryInfoList(QDir::Files);
    Pulsars pulsars = new QVector<Pulsar>;
    for (int i = 0; i < list.size(); i++)
        (*pulsars) += *PulsarReader::ReadPulsarFile(list[i].absoluteFilePath());

    if (pulsars->size())
        FileCompressor::dump(pulsars, QDir(name).absolutePath() + QDir(name).dirName() + ".all.pulsar");
}

void FileCompressor::dump(Pulsars pulsars, QString name) {
    QFile f(name);
    f.open(QIODevice::WriteOnly);

    f.write(("file: " + pulsars->at(0).data.name + "\n").toUtf8());
    f.write("tresolution 0.0999424\n"); // do not try to understand it
    f.write("Start time\tmodule\tray\tdispersion\tperiod\tsnr\n");

    std::sort(pulsars->begin(), pulsars->end(), Pulsar::secondComparator);

    bool filtered = false;
    for (int i = 0; i < pulsars->size(); i++) {
        if (!filtered && pulsars->at(i).filtered) {
            f.write("filtered next\n");
            filtered = true;
        }

        QByteArray d = QString("%1\t%2\t%3\t%4\t%5\t%6\n").
                arg(pulsars->at(i).time()).
                arg(pulsars->at(i).module + 1).
                arg(pulsars->at(i).ray + 1).
                arg(pulsars->at(i).dispersion).
                arg(QString::number(pulsars->at(i).period, 'f', 4)).
                arg(QString::number(pulsars->at(i).snr, 'f', 1)).
                toUtf8();

        f.write(d);
    }

    f.write("additional data:\n");
    QDataStream totalStream(&f);
    for (int i = 0; i < pulsars->size(); i++) {
        QVector<QVariant> pl;
        for (int j = 0; pulsars->at(i).data.data[0][0][0][j] != 0 || pulsars->at(i).data.data[0][0][0][j] != 0; j++)
            pl.push_back(pulsars->at(i).data.data[0][0][0][j]);

        totalStream << QVariant(pl.toList());
    }

    for (int i = 0; i < pulsars->size(); i++) {
        bool flushed = false;
        for (int j = 0; j < i; j++)
            if (pulsars->at(i).ray == pulsars->at(j).ray &&
                    pulsars->at(i).module == pulsars->at(j).module &&
                    pulsars->at(i).nativeTime == pulsars->at(j).nativeTime &&
                    equal(pulsars->at(i).data.data[0][0][0], pulsars->at(j).data.data[0][0][0])) {
                totalStream << QVariant(j);
                break;
                flushed = true;
            }

        if (!flushed) {
            int j = rewind(pulsars->at(i).data.data[0][0][0]);

            QList<QVariant> l;
            while (j < pulsars->at(i).data.npoints)
                l.push_back(pulsars->at(i).data.data[0][0][0][j++]);

            totalStream << QVariant(l);
        }
    }
}

int FileCompressor::rewind(float *data) {
    int j = 0;

    while (data[j] != 0 || data[j + 1] != 0)
        j++;

    while (data[j] == 0)
        j++;

    return j;
}

bool FileCompressor::equal(float *a, float *b) {
    int i = rewind(a);
    int j = rewind(b);

    for (int k = 0; k < 100; k++)
        if (a[i++] != b[j++])
            return false;

    return true;

}
