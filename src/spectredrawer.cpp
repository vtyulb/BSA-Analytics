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

QVector<int> SpectreDrawer::getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period) {
    int start = 0;
    while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > interval / 2)
        start++;

    QVector<double> res;
    //hello pulsar.h::calculateAdditionalData
    for (int offset = -period / data.oneStep / 2; offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1; offset++) {
        double sum = 0;
        int n = 0;
        for (double i = start + offset; i < start + offset  + interval / data.oneStep; i += period / data.oneStep * 2, n++)
            sum += data.data[module][channel][ray][int(i)];

        res.push_back(sum / n);
    }

    double max = 0;
    double min = 0;
    for (int i = 0; i < res.size(); i++) {
        if (max < res[i])
            max = res[i];

        if (min > res[i])
            min = res[i];
    }

    QVector<int> norm;
    for (int i = 0; i < res.size(); i++)
        norm.push_back(255 * (res[i] - min) / (max - min));

    return norm;
}

void SpectreDrawer::drawSpectre(int module, int ray, QString fileName, QTime time, double period) {
    Reader r;
    data = r.readBinaryFile(fileName);

    const int step =  INTERVAL / data.oneStep;
    for (int channel = 0; channel < data.channels - 1; channel++)
        for (int i = 0; i < data.npoints; i += step)
            PulsarWorker::subtract(data.data[module][channel][ray] + i, min(step, data.npoints - i));

    matrix.resize(data.channels - 1);
    for (int i = 0; i < matrix.size(); i++)
        matrix[i] = getAnswer(data, i, module, ray, time, period);

    ui = new Ui::SpectreUI;
    ui->setupUi(this);

    ui->channels->setMaximum(matrix.size());

    QObject::connect(ui->channels, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->time, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->saver, SIGNAL(clicked(bool)), this, SLOT(saveAs()));

    this->reDraw();

    this->show();
}

void SpectreDrawer::reDraw() {
    int x = ui->time->value();
    int y = ui->channels->value();

    QVector<QVector<int> > r;
    r.resize(matrix.size() / y);
    for (int i = 0; i < matrix.size() / y; i++)
        for (int j = 0; j < matrix[0].size() / x; j++) {
            int v = 0;
            for (int k = 0; k < y; k++)
                for (int l = 0; l < x; l++)
                    v += matrix[i * y + k][j * x + l];

            v /= (x * y);
            r[i].push_back(v);
        }

    ui->drawer->spectre = drawImage(r, data);
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
