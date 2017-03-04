#include "spectredrawer.h"

#include <QVector>
#include <QImage>
#include <QDir>
#include <QDesktopServices>
#include <QPainter>
#include <QUrl>
#include <QProcess>
#include <QFileDialog>

#include <reader.h>
#include <pulsarworker.h>
#include <startime.h>
#include <nativespectredrawer.h>

#include <algorithm>

using std::min;

QVector<double> SpectreDrawer::getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period, int startPoint) {
    int start = startPoint + 1;

    if (startPoint == -1)
        while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > interval / 1.5)
            start++;


    QVector<double> res;
    //hello pulsar.h::calculateAdditionalData
    for (int offset = (period < 1000 ? -period / data.oneStep / 2 : -20); offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1; offset++) {
        double sum = 0;
        int n = 0;
        for (double i = start + offset; i < start + offset  + interval / data.oneStep; i += period / data.oneStep * 2, n++)
            sum += data.data[module][channel][ray][int(i + 0.5)];


        res.push_back(sum / n);
        if (res.size() > 40 && period > 1000)
            break;
    }

    return res;
}

void SpectreDrawer::drawSpectre(int module, int ray, QString fileName, QTime time, double period) {
    Reader reader;
    Data data = reader.readBinaryFile(fileName);
    drawSpectre(module, ray, data, time, period);
    data.releaseData();
}

void SpectreDrawer::drawSpectre(int module, int ray, const Data &_data, QTime time, double period, int startPoint) {
    data = _data;
    data.fork();

    this->module = module;
    this->ray = ray;
    this->period = period;
    this->time = time;

    if (period < 1000) {
        const int step =  INTERVAL / data.oneStep;
        for (int channel = 0; channel < data.channels - 1; channel++)
            for (int i = 0; i < data.npoints; i += step)
                PulsarWorker::subtract(data.data[module][channel][ray] + i, min(step, data.npoints - i));
    }

    ui = new Ui::SpectreUI;
    ui->setupUi(this);

    ui->channels->setMaximum(data.channels - 1);

    QObject::connect(ui->channels, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->time, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->dispersion, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->saver, SIGNAL(clicked(bool)), this, SLOT(saveAs()));

    for (int i = 0; i < data.channels; i++)
        r.push_back(getAnswer(data, i, module, ray, time, period, startPoint));

    this->reDraw();
    this->show();
    this->resize(420, 380);
}

void SpectreDrawer::reDraw() {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];

    QVector<QVector<int> > matrix;

    int chs = ui->channels->value();
    int tms = ui->time->value();
    int dsp = ui->dispersion->value();

    for (int  i = 0; i < data.channels / chs - 1; i++) {
        QVector<double> res;
        for (int j = 0; j < r[0].size() - tms; j++) {
            double  sum = 0;
            int n = 0;
            for (int c = i * chs; c < i * chs + chs; c++) {
                    int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dsp * (c - i * chs) / data.oneStep + 0.5);
                    for (int k = 0; k < tms; k++)
                        if (j + dt >= 0) {
                            sum += r[c][j + dt + k];
                            n++;
                        }

                }
            res.push_back(sum / n);
        }


        double max = -1e+20;
        double min = 1e+20;
        for (int i = 0; i < res.size(); i++) {
            if (max < res[i])
                max = res[i];

            if (min > res[i])
                min = res[i];
        }

        QVector<int> norm;
        for (int i = 0; i < res.size(); i++)
            norm.push_back(255 * (res[i] - min) / (max - min));



        matrix.push_back(norm);
    }



    ui->drawer->spectre = drawImage(matrix, data);
    ui->drawer->repaint();
}

QImage SpectreDrawer::drawImage(QVector<QVector<int> > matrix, const Data &data) {
    int nrm = 10;
    const int offset = 50;

    QImage image(matrix[0].size() * nrm + offset, matrix.size() * nrm, QImage::Format_ARGB32);
    QPainter p(&image);
    p.fillRect(0, 0, image.width(), image.height(), Qt::white);

    for (int i = 0; i < matrix.size(); i++)
        for (int j = 0; j < matrix[i].size(); j++) {
            int color = 255 - matrix[i][j];
            p.setPen(QColor(color, color, color));
            p.setBrush(QBrush(QColor(color, color, color)));
            p.drawRect(j * nrm + offset, i * nrm, nrm, nrm);
        }

    p.setPen(QColor("green"));
    for (int i = 0; i < matrix.size(); i++)
        p.drawText(1, i * nrm + nrm - 1, QString::number(data.fbands[i * ui->channels->value()]));


    p.end();

    return image;
}

void SpectreDrawer::saveAs() {
    QString savePath = QFileDialog::getSaveFileName(this, "Spectre");
    if (savePath != "")
        ui->drawer->spectre.save(savePath);
}

SpectreDrawer::~SpectreDrawer() {
    data.releaseData();
}
