#include "flowdetecter.h"
#include "ui_flowdetecter.h"

#include <reader.h>
#include <startime.h>

#include <QMessageBox>

using std::max;
using std::sort;

FlowDetecter::FlowDetecter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlowDetecter)
{
    ui->setupUi(this);

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(run()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

FlowDetecter::~FlowDetecter()
{
    delete ui;
}

void FlowDetecter::run() {
    module = ui->module->value();
    dispersion = ui->dispersion->value();
    ray = ui->ray->value();
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

    int start = 0;
    while (time.secsTo(QTime::fromString(StarTime::StarTime(data, start))) > 90)
        start++;

    QVector<double> profile;
    for (int j = 0; j < period * data.oneStep + 1; j++) {
        double result = 0;
        for (double i = start; i < start + 180 * data.oneStep; i += period * data.oneStep)
            result += res[int(i + 0.5)];

        profile.push_back(result);
    }

    sort(profile.data(), profile.data() + profile.size());

    double final = 0;
    for (int i = profile.size() - 1; i > profile.size() - points - 1; i--)
        final += profile[i];

    final /= points;

    QMessageBox::information(this, "Flow", QString("Res is %1").arg(QString::number(final)));
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
