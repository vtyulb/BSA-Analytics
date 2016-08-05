#include "fouriersearch.h"

#include <QDir>
#include <QStringList>
#include <QDebug>

#include <reader.h>
#include <fourier.h>

void FourierSearch::run(QString pathToCuttedData) {
    QStringList files = QDir(pathToCuttedData).entryList();
    const int modules = 6;
    const int rays = 8;
    const int channels = 6;
    QVector<float> res[6][8];
    for (int i = 0; i < modules; i++)
        for (int j = 0; j < rays; j++) {
            res[i][j].resize(1024);
            res[i][j].fill(0);
        }

    for (int i = 0; i < files.size(); i++)
        if (files[i].endsWith(".pnt")) {
            QString name = QDir(pathToCuttedData).absolutePath()+ "/" + files[i];
            qDebug() << "processing file" << name;
            Data data = Reader().readBinaryFile(name);
            for (int module = 0; module < modules; module++)
                for (int ray = 0; ray < rays; ray++)
                    for (int channel = 0; channel < channels; channel++)
                        runFourier(data, res[module][ray], module, ray, channel);
        }

    QFile f("/home/vlad/txt");
    f.open(QIODevice::WriteOnly);
    QTextStream s(&f);
    for (int i = 0; i < 1024; i++)
        s << res[5][7][i] << "\n";
}

void FourierSearch::runFourier(const Data &data, QVector<float> &res, int module, int ray, int channel) {
    float *dt = new float[1024];
    Fourier::FFTAnalysis(data.data[module][channel][ray], dt, 2048, 1024);
    for (int i = 0; i < 1024; i++)
        res[i] += dt[i];

    delete dt;
}
