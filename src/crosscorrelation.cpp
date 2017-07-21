#include <crosscorrelation.h>
#include <fourier.h>

#include <algorithm>

#include <QDebug>

const int size = 2048;

QVector<float> CrossCorrelation::process(const Data &data, int module, int ray, int offset) {
    QVector<float> res;


    QVector<double> dt(size / 2, 1);
    QVector<float> tmp(size / 2, 0);

    for (int channel = 0; channel < data.channels - 1; channel++) {
        Fourier::FFTAnalysis(data.data[module][channel][ray] + offset, tmp.data(), size, size / 2);
        for (int i = 0; i < size / 2; i++)
            dt[i] *= tmp[i];
    }

    double baseline = findBaseline(data, module, ray, offset);
    double correlation = 0;
    for (int i = 0; i < dt.size(); i++)
        correlation += dt[i];

    double snr = correlation / baseline;
    qDebug() << "point: " << offset << ", snr:" << snr << "baseline:" << baseline << "correlation" << correlation;

    return res;
}

double CrossCorrelation::findBaseline(const Data &data, int module, int ray, int offset) {
    QVector<float> example(size, 1);
    for (int i = 0; i < size; i++) {
        QVector<float> tmp;
        for (int j = 0; j < 32; j++)
            tmp.push_back(data.data[module][j][ray][(i + j) % size + offset]);

        std::sort(tmp.begin(), tmp.end());
        tmp[31] = tmp[29];
        tmp[30] = tmp[29];
        double cr = 1;
        for (int j = 0; j < 32; j++)
            example[i] *= tmp[j];
    }

    QVector<float> fourier(size / 2, 0);
    Fourier::FFTAnalysis(example.data(), fourier.data(), size, size / 2);

    double res = 0;
    for (int i = 0; i < size / 2; i++)
        res += fourier[i];

    return res;
}
