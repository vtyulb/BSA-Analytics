#include "spectredrawer.h"

#include <QVector>
#include <QImage>
#include <QDir>
#include <QDesktopServices>
#include <QPainter>
#include <QUrl>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <reader.h>
#include <pulsarworker.h>
#include <startime.h>
#include <nativespectredrawer.h>

#include <algorithm>

using std::min;

SpectreDrawer::SpectreDrawer():
    QWidget(NULL)
{
    addFromMem = false;
    restoreGeometry(QSettings().value("SpectreGeometry").toByteArray());
    setAttribute(Qt::WA_DeleteOnClose);
}

SpectreDrawer::~SpectreDrawer() {
    QSettings().setValue("SpectreGeometry", saveGeometry());
    delete ui;
}

QVector<double> SpectreDrawer::getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period, int startPoint) {
    int start = startPoint + 1;

    if (startPoint == -1)
        while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > interval / 1.5)
            if (start > data.npoints) {
                QMessageBox::warning(this, "Error", "Time is invalid!\n"
                                                    "There is no points inside data with these time.");
                deleteLater();
                return QVector<double>();
            } else
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
    QObject::connect(&reader, SIGNAL(progress(int)), Settings::settings()->getProgressBar(), SLOT(setValue(int)));
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
    ui->dispersion->setValue(std::max(Settings::settings()->dispersion(), 0.0));

    ui->channels->setMaximum(data.channels - 1);

    QObject::connect(ui->channels, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->time, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->dispersion, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
    QObject::connect(ui->saver, SIGNAL(clicked(bool)), this, SLOT(saveAs()));
    QObject::connect(ui->memPlus, SIGNAL(clicked()), this, SLOT(memPlus()));
    QObject::connect(ui->mem, SIGNAL(clicked()), this, SLOT(mem()));

    for (int i = 0; i < data.channels; i++) {
        r.push_back(getAnswer(data, i, module, ray, time, period, startPoint));
        if (r[i].size() == 0)
            return;
    }

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

    rawRes.clear();
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

        rawRes.push_back(res);
    }

    rotateMatrix();
    if (addFromMem) {
        addFromMem = false;
        QList<QVariant> lastSpectre = QSettings().value("LastSpectre").toList();
        if (lastSpectre.size() < rawRes.size() * rawRes[0].size()) {
            QMessageBox::warning(this, "Error", "Memory is not filled!");
        } else {
            for (int i = 0; i < rawRes.size(); i++)
                for (int j = 0; j < rawRes[i].size(); j++)
                    rawRes[i][j] += lastSpectre[i * rawRes[0].size() + j].toDouble();
            mem();
        }
    }

    for (int channel = 0; channel < data.channels / chs - 1; channel++) {
        double max = rawRes[channel][0];
        double min = rawRes[channel][0];
        for (int i = 0; i < rawRes[channel].size(); i++) {
            if (max < rawRes[channel][i])
                max = rawRes[channel][i];

            if (min > rawRes[channel][i])
                min = rawRes[channel][i];
        }

        QVector<int> norm;
        for (int i = 0; i < rawRes[channel].size(); i++)
            norm.push_back(255 * (rawRes[channel][i] - min) / (max - min));

        matrix.push_back(norm);
    }

    ui->drawer->spectre = drawImage(matrix, data);
    ui->drawer->repaint();
}

QImage SpectreDrawer::drawImage(QVector<QVector<int> > matrix, const Data &data) {
    int nrm = 10;
    const int offset = 50;

    QImage image(matrix[0].size() * nrm + offset, matrix.size() * nrm, QImage::Format_ARGB32);
    ui->drawer->setMinimumSize(std::min(image.width(), 1024), image.height());
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

void SpectreDrawer::rotateMatrix() {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];

    int dsp = ui->dispersion->value();
    int maxAt = 0;
    double maxRes = 1e-10;
    for (int i = 0; i < rawRes[0].size(); i++) {
        double cur = 0;
        for (int channel = 0; channel < rawRes.size(); channel++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dsp * channel / data.oneStep + 0.5);
            int v = (i + dt + rawRes[0].size() * 100) % rawRes[0].size();
            cur += rawRes[channel][v];
        }

        if (cur > maxRes) {
            maxAt = i;
            maxRes = cur;
        }
    }

    int rotateCount = (rawRes[0].size() * 100000 - maxAt - 1) % rawRes[0].size();
    for (int channel = 0; channel < rawRes.size(); channel++)
        std::rotate(rawRes[channel].begin(), rawRes[channel].begin() + rotateCount, rawRes[channel].end());
}

void SpectreDrawer::saveAs() {
    QString savePath = QFileDialog::getSaveFileName(this, "Spectre");
    if (savePath != "")
        ui->drawer->spectre.save(savePath);
}

void SpectreDrawer::mem() {
    QList<QVariant> lastSpectre;
    for (int i = 0; i < rawRes.size(); i++)
        for (int j = 0; j < rawRes[i].size(); j++)
            lastSpectre.push_back(rawRes[i][j]);

    QSettings().setValue("LastSpectre", lastSpectre);
}

void SpectreDrawer::memPlus() {
    addFromMem = true;
    reDraw();
}
