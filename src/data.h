#ifndef DATA_H
#define DATA_H

#include <QDateTime>
#include <QString>

struct Data {
    int npoints;
    int modules;
    int channels;
    int rays;
    float ****data;

    QDateTime time;

    QString name;
    double oneStep;
    double delta_lucha;
    double *fbands;
    double stairSize;
    double sigma;

    bool releaseProtected;

    void releaseData() {
        if (releaseProtected)
            return;

        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                for (int k = 0; k < rays; k++)
                    delete[] data[i][j][k];

        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                delete[] data[i][j];

        for (int i = 0; i < modules; i++)
            delete[] data[i];

        delete[] data;
        delete[] fbands;
    }

    void halfRelease(int module, int ray) {
        releaseProtected = true;
        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                for (int k = 0; k < rays; k++)
                    if (i != module || k != ray)
                        delete[] data[i][j][k];
    }

    bool isLong() {
        return oneStep < 0.02;
    }

    double stairHeight(int module, int ray, int channel) {
        int f1 = 3030;
        int t1 = 3050;

        int f2 = 3085;
        int t2 = 3105;

        double c = 99.9424 / 12.4928;

        if (isLong()) {
            f1 *= c;
            t1 *= c;
            f2 *= c;
            t2 *= c;
        }

        double resMin = 0;
        double resMax = 0;

        for (int i = f1; i < t1; i++)
            resMin += data[module][channel][ray][i];

        resMin /= (t1 - f1);

        for (int i = f2; i < t2; i++)
            resMax += data[module][channel][ray][i];

        resMax /= (t2 - f2);

        return resMax - resMin;
    }

    void init() {
        sigma = -1;
        stairSize = 0;
        releaseProtected = false;
        data = new float***[modules];
        fbands = new double[channels];
        for (int j = 0; j < modules; j++) {
            data[j] = new float**[channels];
            for (int i = 0; i < channels; i++) {
                data[j][i] = new float*[rays];
                for (int k = 0; k < rays; k++)
                    data[j][i][k] = new float[npoints];
            }
        }
    }
};



#endif // DATA_H
