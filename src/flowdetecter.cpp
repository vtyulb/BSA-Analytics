#include "flowdetecter.h"
#include "ui_flowdetecter.h"

#include <reader.h>
#include <startime.h>
#include <pulsarworker.h>
#include <spectredrawer.h>

#include <QMessageBox>
#include <QFileDialog>

using std::max;
using std::sort;

FlowDetecter::FlowDetecter(int module, int dispersion, int ray, int points, bool trackImpulses,
                           int sensitivity, double period, QTime time, QString fileName, QObject *parent):
    module(module),
    dispersion(dispersion),
    ray(ray),
    points(points),
    trackImpulses(trackImpulses),
    sensitivity(sensitivity),
    period(period),
    time(time),
    fileName(fileName),
    QObject(parent)
{
}

void FlowDetecter::run() {
    Reader r;
    data = r.readBinaryFile(fileName);

    Settings::settings()->loadStair();

    res = applyDispersion();
    for (int i = 0; i < res.size() - 20; i += 20)
        subtract(res.data() + i, 20);

    int start = 0;
    while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > 90)
        start++;

    QVector<double> profile;
    int maximumAt = 0;
    double maximum = 0;
    for (int j = 0; j < period / data.oneStep + 1; j++) {
        double result = 0;
        int number = 0;
        for (double i = start + j; i < start + 180 / data.oneStep; i += period / data.oneStep) {
            result += res[int(i + 0.5)];
            number++;
        }

        if (maximum < result / number) {
            maximum = result / number;
            maximumAt = j;
        }
        profile.push_back(result / number);
    }

    if (trackImpulses) {
        int impulses = 0;
        for (double i = start + maximumAt; i < start + 180 / data.oneStep; i += period / data.oneStep)
            if (maximum * sensitivity < res[int(i + 0.5)]) {

                double v1 = data.fbands[0];
                double v2 = data.fbands[1];

                /*bool stop = false;

                for (int k = 0; k < data.channels; k++) {
                    int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * k / data.oneStep + 0.5);
                    double v = data.data[module][k][ray][int(i + 0.5) + dt];
                    if (v > res[int(i + 0.5)] / data.channels * 10)
                        stop = true;
                }

                if (stop)
                    continue;*/

                static bool forceStop = false;
                if (!forceStop) {
                    SpectreDrawer drawer;
                    drawer.drawSpectre(module, ray, data, QTime::fromString(StarTime::StarTime(data, i), "HH:mm:ss"), 100500, i);

                    QMessageBox question(0);
                    question.setWindowModality(Qt::NonModal);
                    question.setWindowTitle("Found big impulse!");
                    question.setText("Found big impulse at point " + QString::number(int(i + 0.5)));
//                    question.addButton(QString("Continue"), QMessageBox::AcceptRole);
//                    question.addButton(QString("Stop"), QMessageBox::RejectRole);
                    question.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    question.setModal(false);

                    int res = question.exec();

                    if (res != QMessageBox::Ok)
                        break;
                }
            }
    }



    sort(profile.data(), profile.data() + profile.size());

    double final = 0;
    for (int i = profile.size() - 1; i > profile.size() - points - 1; i--)
        final += profile[i];

    final /= points;

    QMessageBox::information(NULL, "Flow", QString("Res is %1").arg(QString::number(final)));
    data.releaseData();
}

QVector<double> FlowDetecter::applyDispersion() {
    // Hello pulsarworker.cpp :: PulsarWorker::applyDispersion
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    double mxd = (4.1488) * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion;
    mxd *= -data.channels;
    mxd /= data.oneStep;

    QVector<double> res(data.npoints, 0);

    for (int i = 0; i < data.npoints - mxd; i++)
        for (int j = 0; j < data.channels - 1; j++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * j / data.oneStep + 0.5);
            res[i] += data.data[module][j][ray][max(i + dt, 0)];
        }

    for (int i = data.npoints - mxd; i < data.npoints; i++)
        for (int j = 0; j < data.channels - 1; j++)
            res[i] += data.data[module][j][ray][i];

    return res;
}

void FlowDetecter::subtract(double *res, int size) {
    // Hello, void PulsarWorker::subtract(real *res, int size)
    double a = 0;
    double b = 0;
    for (int i = 0; i < size / 2; i++)
        a += res[i] / (size / 2);

    for (int i = 1; i <= size / 2; i++)
        b += res[size - i] / (size / 2);

    for (int i = 0; i < size; i++)
        res[i] -= (b - a) * i / size + a;
}

