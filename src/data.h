#ifndef DATA_H
#define DATA_H

#include <QDateTime>
#include <QString>
#include <QVector>

struct Data {
    int npoints;
    int modules = -1;
    int channels;
    int rays;
    float ****data;

    QDateTime time;

    QString previousLifeName;
    QString name;
    QString message;
    double oneStep;
    double delta_lucha;
    QVector<double> fbands;
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
    }

    void halfRelease(int module, int ray) {
        releaseProtected = true;
        for (int i = 0; i < modules; i++)
            for (int j = 0; j < channels; j++)
                for (int k = 0; k < rays; k++)
                    if (i != module || k != ray)
                        delete[] data[i][j][k];
    }

    bool isLong() const {
        return oneStep < 0.02;
    }

    double stairHeight(int module, int ray, int channel) const {
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

    void fork() {
        Data tmp = *this;
        init();
        for (int j = 0; j < modules; j++)
            for (int i = 0; i < channels; i++)
                for (int k = 0; k < rays; k++)
                    for (int z = 0; z < npoints; z++)
                        data[j][i][k][z] = tmp.data[j][i][k][z];
    }

    void init() {
        sigma = -1;
        releaseProtected = false;
        data = new float***[modules];
        for (int j = 0; j < modules; j++) {
            data[j] = new float**[channels];
            for (int i = 0; i < channels; i++) {
                data[j][i] = new float*[rays];
                for (int k = 0; k < rays; k++)
                    data[j][i][k] = new float[npoints];
            }
        }
    }

    QDate dateFromPreviousLifeName() const {
        QString res;
        if (previousLifeName.endsWith("pnthr"))
            res = previousLifeName.right(21);
        else
            res = previousLifeName.right(19);

        res = res.left(6);
        QDate date = QDate::fromString(res, "ddMMyy");
        if (date.year() < 2000)
            date.setDate(date.year() + 100, date.month(), date.day());

        return date;
    }

    int hourFromPreviousLifeName() const {
        QString res;
        if (previousLifeName.endsWith("pnthr"))
            res = previousLifeName.right(21);
        else
            res = previousLifeName.right(19);

        res = res.left(9).right(2);
        return res.toInt();
    }

    bool isValid() const {
        return modules != -1;
    }
};

static inline QDateTime stringDateToDate(QString name) {
    int day = name.left(2).toInt();
    int month = name.left(4).right(2).toInt();
    int year = name.left(6).right(2).toInt();
    int hour = name.left(9).right(2).toInt();
    int ground = name.left(12).right(1).toInt();

    year += ground * 100;

    return QDateTime(QDate(year, month, day), QTime(hour, 0));
}



#endif // DATA_H
