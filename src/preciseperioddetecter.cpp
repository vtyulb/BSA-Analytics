#include <preciseperioddetecter.h>
#include <pulsarworker.h>
#include <reader.h>
#include <settings.h>
#include <mainwindow.h>

#include <QDebug>
#include <QApplication>

#include <math.h>

double PrecisePeriodDetecter::phase1 = 0;
double PrecisePeriodDetecter::phase2 = 0;
double PrecisePeriodDetecter::phase3 = 0;

void PrecisePeriodDetecter::detect(QString file1, QString file2, QString file3, int module, int ray, int dispersion, double period, QTime time) {

    Settings::settings()->setPreciseSearch(true);
    Settings::settings()->setPeriod(period);
    Settings::settings()->setSinglePeriod(true);
    Settings::settings()->setIntellectualFilter(false);
    Settings::settings()->setTime(time);


    Reader r;
    Data data1 = r.readBinaryFile(file1);
    PulsarWorker worker1(module, ray, dispersion, data1);
    worker1.run();

    Data data2 = r.readBinaryFile(file2);
    PulsarWorker worker2(module, ray, dispersion, data2);
    worker2.run();

    Data data3 = r.readBinaryFile(file3);
    PulsarWorker worker3(module, ray, dispersion, data3);
    worker3.run();


    phase1 = getPhase(data1, worker1.res.at(0).firstPoint);
    phase2 = getPhase(data2, worker2.res.at(0).firstPoint);
    phase3 = getPhase(data3, worker3.res.at(0).firstPoint);

    double cv = period;



    QVector<double> res;
    for (double p = period - 0.0001; p < period + 0.0001; p += 1e-10) {
        if (check(cv * 1000) > check(p * 1000))
            cv = p;

        if (check(p * 1000) < 500)
            if (check(p * 1000) < check((p + 1e-9)*1000) &&
                check(p * 1000) < check((p - 1e-9)*1000))
                qDebug() << "good value is" << QString::number(p, 'g', 9) << "with error" <<
                            check(p * 1000);

        res.push_back(-check(p * 1000));
    }

    Data data;
    data.channels = 1;
    data.modules = 1;
    data.rays = 1;
    data.npoints = res.size();
    data.init();
    for (int i = 0; i < res.size(); i++)
        data.data[0][0][0][i] = res[i];

    MainWindow *w = new MainWindow;
    w->show();
    w->regenerate(data);

    qDebug() << cv << "result is" << check(cv * 1000);
    qDebug() << "dump value" << period - 0.0001;
}

double PrecisePeriodDetecter::getPhase(Data data, int point) {
    return data.time.toMSecsSinceEpoch() + data.oneStep * point * 1000;
}

double PrecisePeriodDetecter::check(double period) {
    int a = fabs(phase2 - phase1) / period + 0.5;
    int b = fabs(phase3 - phase1) / period + 0.5;
    int c = fabs(phase3 - phase2) / period + 0.5;
    return fabs(a * period - fabs(phase2 - phase1)) +
            fabs(b * period - fabs(phase3 - phase1)) +
            fabs(c * period - fabs(phase3 - phase2));
}
