#include "flowdetecter.h"

#include <reader.h>
#include <startime.h>
#include <pulsarworker.h>
#include <spectredrawer.h>

#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

using std::max;
using std::min;
using std::sort;

FlowDetecter::FlowDetecter(int module, int dispersion, int ray, int points, bool trackImpulses, int sensitivity,
                           double period, QTime time, QString fileName, QObject *parent):
    QObject(parent),
    module(module),
    dispersion(dispersion),
    ray(ray),
    points(points),
    trackImpulses(trackImpulses),
    sensitivity(sensitivity),
    period(period),
    time(time),
    fileName(fileName)
{
}

void FlowDetecter::run() {
    Reader r;
    data = r.readBinaryFile(fileName);

    Settings::settings()->loadStair();

    for (int channel = 0; channel < data.channels; channel++)
        for (int i = 0; i < data.npoints; i++)
            data.data[module][channel][ray][i] /= Settings::settings()->getStairHeight(module, ray, channel) / 2100;

    const int subtractStep = 5 / data.oneStep;
    for (int i = 0; i < data.npoints - subtractStep / data.oneStep; i += subtractStep)
        for (int channel = 0; channel < data.channels; channel++) {
            float mn = 1e+100;
            for (int k = i; k < i + subtractStep; k++)
                mn = min(mn, data.data[module][channel][ray][k]);

            for (int k = i; k < i + subtractStep; k++)
                data.data[module][channel][ray][k] -= mn;
        }

    res = applyDispersion();

    int start = 0;
    while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > 90)
        start++;

    QVector<double> profile;
    QString profileString;
    int maximumAt = 0;
    double maximum = 0;
    for (int j = 0; j < period / data.oneStep + 1; j++) {
        double result = 0;
        int count = 0;
        for (double i = start + j; i < start + 180 / data.oneStep; i += period / data.oneStep) {
            result += res[int(i + 0.5)];
            count++;
        }

        if (maximum < result / count) {
            maximum = result / count;
            maximumAt = j;
        }
        profile.push_back(result / count);
        profileString += QString::number(result / count) + " ";
    }

    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    double average = 0;

    QString resString;
    for (int channel = 0; channel < data.channels - 1; channel++) {
        double res = 0;
        int count = 0;
        int offset = 4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * channel / data.oneStep + 0.5;
        for (double i = start + maximumAt; i < start + maximumAt + 180 / data.oneStep; i += period / data.oneStep) {
            res += data.data[module][channel][ray][int(i + offset + 0.5)];
            count++;
        }
        average += res / count;
        resString += " " + QString::number(res / count);
    }

    average /= data.channels - 1;
    qApp->clipboard()->setText(QString::number(average) + " " + resString);
    QMessageBox::information(NULL, "Flow", QString("Average flow is %1\n"
                                                   "By channels: %2\n"
                                                   "One period profile: %3").arg(QString::number(average))
                                                                     .arg(resString).arg(profileString));

    if (trackImpulses) {
        bool forceStop = false;
        for (double i = start + maximumAt; i < start + 180 / data.oneStep; i += period / data.oneStep)
            if (maximum * sensitivity < res[int(i + 0.5)])
                if (!forceStop) {
                    SpectreDrawer *drawer = new SpectreDrawer;
                    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Close);
                    QDialog *impulseDialog = new QDialog;
                    impulseDialog->restoreGeometry(QSettings().value("ImpulseDialogGeometry").toByteArray());
                    impulseDialog->setWindowTitle("Found big impulse!");

                    QVBoxLayout *layout = new QVBoxLayout(impulseDialog);
                    layout->addWidget(new QLabel("Found big impulse at point " + QString::number(int(i + 0.5)) +
                                                 "\n" + QString::number(res[int(i+0.5)]/maximum) + " times bigger than average impulse"));
                    layout->addWidget(drawer);
                    layout->addWidget(buttons);

                    QObject::connect(buttons, SIGNAL(accepted()), impulseDialog, SLOT(accept()));
                    QObject::connect(buttons, SIGNAL(rejected()), impulseDialog, SLOT(reject()));
                    drawer->drawSpectre(module, ray, data, QTime::fromString(StarTime::StarTime(data, i), "HH:mm:ss"), 100500, i);

                    if (impulseDialog->exec() == QDialog::Rejected)
                        forceStop = true;

                    QSettings().setValue("ImpulseDialogGeometry", impulseDialog->saveGeometry());
                    delete impulseDialog;
                }
    }

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
