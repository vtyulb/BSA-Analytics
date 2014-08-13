#ifndef DATA_H
#define DATA_H

#include <QDateTime>
struct Data {
    int npoints;
    int modules;
    int channels;
    int rays;
    float ****data;

    QDateTime time;
    double oneStep;
    double delta_lucha;

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
    }
};



#endif // DATA_H
