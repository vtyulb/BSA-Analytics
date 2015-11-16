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

FlowDetecter::FlowDetecter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlowDetecter)
{
    ui->setupUi(this);

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(run()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    QObject::connect(ui->stairButton, SIGNAL(clicked(bool)), this, SLOT(setStairFileName()));
    QObject::connect(ui->fileButton, SIGNAL(clicked(bool)), this, SLOT(setFileName()));
}

void FlowDetecter::setStairFileName() {
    ui->stairFile->setText(QFileDialog::getOpenFileName(this));
}

void FlowDetecter::setFileName() {
    ui->file->setText(QFileDialog::getOpenFileName(this));
}

FlowDetecter::~FlowDetecter()
{
    delete ui;
}

void FlowDetecter::run() {
    module = ui->module->value() - 1;
    dispersion = ui->dispersion->value();
    ray = ui->ray->value() - 1;
    time = ui->timeEdit->time();

    points = ui->point->value();
    period = ui->period->value();

    Reader r1;
    Data stair = r1.readBinaryFile(ui->stairFile->text());

    Reader r2;
    data = r2.readBinaryFile(ui->file->text());

    double K = 0;
    for (int i = 0; i < data.channels; i++)
        K += stair.stairHeight(module, ray, i);

    K /= data.channels;
    K = 2400 / K;

    for (int i = 0; i < data.channels; i++)
        for (int j = 0; j < data.npoints; j++)
            data.data[module][i][ray][j] *= K;

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

    if (ui->impulses->isChecked()) {
        int impulses = 0;
        for (double i = start + maximumAt; i < start + 180 / data.oneStep; i += period / data.oneStep)
            if (maximum * ui->impulseSensitivity->value() < res[int(i + 0.5)]) {

                double v1 = data.fbands[0];
                double v2 = data.fbands[1];

                bool stop = false;

                for (int k = 0; k < data.channels; k++) {
                    int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * k / data.oneStep + 0.5);
                    double v = data.data[module][k][ray][int(i + 0.5) + dt];
                    if (v > res[int(i + 0.5)] / data.channels * 10)
                        stop = true;
                }

                if (stop)
                    continue;

                impulses++;
                if (impulses < 3) {
                    SpectreDrawer drawer;
                    drawer.drawSpectre(module, ray, data, QTime::fromString(StarTime::StarTime(data, i), "HH:mm:ss"), 100500);
                    QMessageBox::information(this, "Found an impulse!", "Found big impulse at point " + QString::number(int(i + 0.5)));

                } else if (impulses == 3)
                    QMessageBox::information(this, "Found an impulse!", "Too many impulses. No more windows at this session!");
            }
    }



    sort(profile.data(), profile.data() + profile.size());

    double final = 0;
    for (int i = profile.size() - 1; i > profile.size() - points - 1; i--)
        final += profile[i];

    final /= points;

    QMessageBox::information(this, "Flow", QString("Res is %1").arg(QString::number(final)));
    stair.releaseData();
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

