#include <crosscorrelation.h>
#include <fourier.h>

#include <algorithm>
#include <math.h>
#include <mainwindow.h>
#include <pulsarworker.h>

#include <QDebug>
#include <QApplication>

const int window = CROSS_CORRELATION_WINDOW;
const int size = CROSS_CORRELATION_SIZE;
const int FIRST_DISPERSION = CROSS_CORRELATION_FIRST_DISPERSION;
const int LAST_DISPERSION = CROSS_CORRELATION_LAST_DISPERSION;
const double v1 = 109.0390625;
const double v2 = 109.1171875;


QVector<double> CrossCorrelation::process(const Data &data, int module, int ray, int offset) {
    QVector<double> res;

    for (int i = 0; i < FIRST_DISPERSION; i++)
        res.push_back(0);

    for (int i = 0; i < 32; i++)
        profile[i].clear();

    for (int i = 0; i < size - window; i += window)
        for (int channel = 0; channel < 32; channel++) {
            float cur = 0;
            for (int k = 0; k < window; k++)
                cur += data.data[module][channel][ray][offset + i + k];

            cur /= window;
            profile[channel].push_back(cur);
        }

    for (int disp = FIRST_DISPERSION; disp <= LAST_DISPERSION / window; disp++)
        res.push_back(correlation(disp));

    subtractNull(res);

    Data dt;
    dt.modules = 1;
    dt.channels = 1;
    dt.rays = 1;
    dt.npoints = res.size();
    dt.init();
    for (int i = 0; i < res.size(); i++)
        dt.data[0][0][0][i] = res[i];

    /*char *argv[2];
    int c = 0;
    QApplication *app = new QApplication(c, argv);
    MainWindow *wnd = new MainWindow;
    wnd->regenerate(dt);
    wnd->show();
    app->exec();*/

    return res;
}

double CrossCorrelation::correlation(int disp) {
    double res = 0;
    int sz = profile[0].size();
    int msz = sz * 10000;
    for (int i = 0; i < sz; i++) {
        QVector<float> tmp;
        tmp.reserve(32);
        for (int j = 0; j < 32; j++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * disp * j / 0.0124928 + 0.5);
            tmp.push_back(profile[j][(i+dt+msz) % sz]);
        }

        std::sort(tmp.begin(), tmp.end());

        double cur = 1;
        for (int j = 2; j < 30; j++)
            cur *= tmp[j];

        res += cur;
    }

    return res * window;
}

void CrossCorrelation::subtractNull(QVector<double> &data) {
    double curv = 0;
    int subWin = 50;
    for (int i = 0; i < subWin; i++)
        curv += data[i] / subWin;

    QVector<double> res = data;
    for (int i = subWin / 2; i < data.size() - subWin / 2; i++) {
        res[i] -= curv;
        curv -= data[i - subWin / 2] / subWin;
        curv += data[i + subWin / 2] / subWin;
    }

    for (int i = 0; i < subWin / 2; i++) {
        res[i] = 0;
        res[res.size() - 1 - i] = 0;
    }

    data = res;
}

Data CrossCorrelation::determinePreciseInterval(const Data &data, int &dispersion) {
    int mxd = abs(int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * 32 / 0.0124928 + 0.5)) + 10;

    double globalMax = -1e+10;
    int bestOffset = 0;
    int bestDispersion = 0;

    for (int offset = 0; offset < data.npoints - mxd; offset++) {
        for (int i = 0; i < 32; i++)
            profile[i].clear();

        for (int i = 0; i < mxd; i++)
            for (int channel = 0; channel < 32; channel++)
                profile[channel].push_back(data.data[0][channel][0][offset + i]);

        for (int disp = dispersion - 5; disp < dispersion + 5; disp++) {
            double corr = correlation(disp);
            if (corr > globalMax) {
                globalMax = corr;
                bestOffset = offset;
                bestDispersion = disp;
            }
        }
    }

    Data res = data;
    res.npoints = mxd;
    res.fork();
    for (int i = 0; i < mxd; i++)
        for (int channel = 0; channel < 33; channel++)
            res.data[0][channel][0][i] = data.data[0][channel][0][i + bestOffset];

    dispersion = bestDispersion;
    return res;
}
