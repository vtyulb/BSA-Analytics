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

    void releaseData() {
        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                for (int k = 0; k < rays; k++)
                    delete data[i][j][k];

        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                delete data[i][j];

        for (int i = 0; i < modules; i++)
            delete data[i];

        delete data;
        delete fbands;
    }

    void init() {
        stairSize = 0;
        data = new float***[modules];
        fbands = new double[channels - 1];
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
