#include "spectredrawer.h"

#include <QAction>
#include <QButtonGroup>
#include <QVector>
#include <QImage>
#include <QDir>
#include <QDesktopServices>
#include <QPainter>
#include <QUrl>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>

#include <reader.h>
#include <pulsarworker.h>
#include <startime.h>
#include <nativespectredrawer.h>

#include <algorithm>

using std::min;

const int nrm = 10;
const int offset = 50;

SpectreDrawer::SpectreDrawer():
    QWidget(nullptr),
    colorModel(grayScale),
    ui(nullptr)
{
    addFromMem = false;
    isWorking = false;
    restoreGeometry(QSettings().value("SpectreGeometry").toByteArray());
    setAttribute(Qt::WA_DeleteOnClose);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

SpectreDrawer::~SpectreDrawer() {
    QSettings().setValue("SpectreGeometry", saveGeometry());
    delete ui;
}

int SpectreDrawer::findFirstPoint(int startPoint) {
    int start = startPoint + 1;

    if (startPoint == -1) {
        while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > interval / 1.5 * (period < 1000))
            if (start > data.npoints) {
                QMessageBox::warning(this, "Error", "Time is invalid!\n"
                                                    "There is no points inside data with these time.");
                deleteLater();
                return 0;
            } else
                start++;
    }

    return start;
}

QVector<double> SpectreDrawer::getAnswer(const Data &data, int channel, int start) {
    QVector<double> res;
    //hello pulsar.h::calculateAdditionalData
    for (int offset = (period < 1000 ? -period / data.oneStep / 2 : -20); offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1; offset++) {
        qApp->processEvents();
        double sum = 0;
        int n = 0;
        for (double i = start + offset; i < start + offset  + interval / data.oneStep; i += period / data.oneStep * 2, n++)
            sum += data.data[module][channel][ray][int(i + 0.5)];


        res.push_back(sum / n);
        if (res.size() > 200 && period > 1000)
            break;
    }

    return res;
}

void SpectreDrawer::drawSpectre(int module, int ray, QString fileName, QTime time, double period) {
    Reader reader;
    QObject::connect(&reader, SIGNAL(progress(int)), Settings::settings(), SLOT(setProgress(int)));
    Data data = reader.readBinaryFile(fileName);
    drawSpectre(module, ray, data, time, period);
    data.releaseData();
}

void SpectreDrawer::drawSpectre(int module, int ray, const Data &_data, QTime time, double period, int startPoint) {
    isWorking = true;
    if (data.isValid())
        data.releaseData();

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

    if (!ui) {
        ui = new Ui::SpectreUI;
        ui->setupUi(this);
        QButtonGroup *group = new QButtonGroup;
        group->addButton(ui->local);
        group->addButton(ui->global);

        ui->local->setChecked(true);

        if (!Settings::settings()->transientAnalytics())
            ui->hideButton->hide();

        ui->modelComboBox->addItem("   GrayScale");
        ui->modelComboBox->addItem("   Jet");
        ui->modelComboBox->addItem("   Thermal");

        QObject::connect(group, SIGNAL(buttonClicked(int)), this, SLOT(reDraw()));
        QObject::connect(ui->channels, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
        QObject::connect(ui->time, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));
        QObject::connect(ui->showDispersion, SIGNAL(clicked()), this, SLOT(reDrawDispersion()));
        QObject::connect(ui->saver, SIGNAL(clicked(bool)), this, SLOT(saveAs()));
        QObject::connect(ui->memPlus, SIGNAL(clicked()), this, SLOT(memPlus()));
        QObject::connect(ui->mem, SIGNAL(clicked()), this, SLOT(mem()));
        QObject::connect(ui->hideButton, SIGNAL(clicked()), this, SLOT(hide()));
        QObject::connect(ui->modelComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeModel(int)));
        QObject::connect(ui->shiftDispersion, SIGNAL(valueChanged(int)), this, SLOT(reDraw()));

        QAction *saveAsAction = new QAction("Save as...", this);
        QObject::connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));
        ui->drawer->addAction(saveAsAction);
    }

    if (!Settings::settings()->transientAnalytics())
        if (ui)
            ui->showDispersion->hide();

    QObject::disconnect(ui->dispersion, SIGNAL(valueChanged(double)), this, SLOT(reDrawDispersion()));
    ui->dispersion->setValue(std::max(Settings::settings()->dispersion(), 0.0));
    QObject::connect(ui->dispersion, SIGNAL(valueChanged(double)), this, SLOT(reDrawDispersion()));

    ui->channels->setMaximum(data.channels - 1);

    r.clear();
    int start = findFirstPoint(startPoint);
    if (startPoint == 0) {
        r.resize(data.channels);
        for (int i = 0; i < data.channels; i++)
            for (int j = 0; j < data.npoints; j++)
                r[i].push_back(data.data[0][i][0][j]);
    } else {
        for (int i = 0; i < data.channels; i++) {
            r.push_back(getAnswer(data, i, start));
            if (r[i].size() == 0)
                return;
        }
    }

    if (Settings::settings()->dispersion() > 200) {
        ui->time->setValue(Settings::settings()->dispersion() / 100);
        ui->time->setMinimum(Settings::settings()->dispersion() / 100);
    }

    this->reDraw();
    this->show();
    qApp->processEvents();
    isWorking = false;
}

void SpectreDrawer::reDraw() {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    if (v1 < 1e-6 || v2 < 1e-6) {
        v1 = 1;
        v2 = 1;
    }

    if (fabs(data.oneStep) < 0.0001)
        data.oneStep = 0.0124928;

    QVector<QVector<double> > matrix;

    int chs = ui->channels->value();
    int tms = ui->time->value();
    double dsp = ui->dispersion->value();

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

            if (!ui || ui->dispersion->value() <= 200)
                res.push_back(sum / n);
            else if (ui) {
                int dispersion = ui->dispersion->value();
                int ratio = dispersion / 100;
                if (j % ratio == 0)
                    res.push_back(sum / n);

                ui->message->setText(QString("Showing 1 of every %1 points").arg(ratio));
            }

        }

        rawRes.push_back(res);
    }

    if (ui->dispersion->value() != 0 && !Settings::settings()->transientAnalytics())
        rotateMatrix();

    if (addFromMem) {
        addFromMem = false;
        QList<QVariant> lastSpectre = QSettings().value("LastSpectre").toList();
        if (lastSpectre.size() < rawRes.size() * (rawRes[0].size() - 5)) {
            QMessageBox::warning(this, "Error", "Memory is not filled!");
        } else {
            for (int j = 0; j < rawRes[0].size(); j++)
                for (int i = 0; i < rawRes.size(); i++) {
                    if (j * rawRes.size() + i >= lastSpectre.size())
                        break;
                    rawRes[i][j] += lastSpectre[j * rawRes.size() + i].toDouble();
                }
            mem();
        }
    }

    double glMax = 0;
    double glMin = 1e+50;
    for (int channel = 0; channel < data.channels / chs - 1; channel++)
        for (int i = 0; i < rawRes[channel].size(); i++) {
            if (glMin > rawRes[channel][i])
                glMin = rawRes[channel][i];

            if (glMax < rawRes[channel][i])
                glMax = rawRes[channel][i];
        }

    for (int channel = 0; channel < data.channels / chs - 1; channel++) {
        double max = ui->local->isChecked() ? rawRes[channel][0] : glMax;
        double min = ui->local->isChecked() ? rawRes[channel][0] : glMin;
        for (int i = 0; i < rawRes[channel].size(); i++) {
            if (max < rawRes[channel][i])
                max = rawRes[channel][i];

            if (min > rawRes[channel][i])
                min = rawRes[channel][i];
        }

        QVector<double> norm;
        for (int i = 0; i < rawRes[channel].size(); i++)
            norm.push_back((rawRes[channel][i] - min) / (max - min));

        matrix.push_back(norm);
    }

    ui->drawer->spectre = drawImage(matrix, data);
    ui->drawer->repaint();
}

QImage SpectreDrawer::drawImage(QVector<QVector<double> > matrix, const Data &data) {
    const int maxMinWidth = Settings::settings()->transientAnalytics() ? 480 : 1024;

    QImage image(matrix[0].size() * nrm + offset, matrix.size() * nrm, QImage::Format_ARGB32);
    ui->drawer->setMinimumSize(std::min(image.width(), maxMinWidth), std::min(image.height(), maxMinWidth * image.height() / image.width()));
    QPainter p(&image);
    p.fillRect(0, 0, image.width(), image.height(), Qt::white);

    for (int i = 0; i < matrix.size(); i++)
        for (int j = 0; j < matrix[i].size(); j++) {
            QColor color = jetModel(matrix[i][j]);
            p.setPen(color);
            p.setBrush(QBrush(color));
            p.drawRect(j * nrm + offset, i * nrm, nrm, nrm);
        }

    p.setPen(QColor("green"));
    for (int i = 0; i < matrix.size(); i++)
        p.drawText(1, i * nrm + nrm - 1, QString::number(data.fbands[i * ui->channels->value()]));

    p.end();

    return drawDispersion(image);
}

QImage SpectreDrawer::drawDispersion(QImage src) {
    static QImage prev = src;
    if (src.isNull())
        src = prev;
    prev = src;

    QPainter p(&src);
    if (ui->showDispersion->isChecked()) {
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(QBrush("red"), 3 + ui->dispersion->value() / 200, Qt::SolidLine));
        double v1 = data.fbands[0];
        double v2 = data.fbands[1];
        double dsp = 4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * 32 * ui->dispersion->value() / 0.0124928;
        if (ui->dispersion->value() > 200)
            dsp /= (ui->dispersion->value() / 100);

        p.drawLine(offset - ui->shiftDispersion->value() + ((src.width()-offset)/nrm - 10 + 0.5) * nrm,
                   nrm / 2,
                   offset - ui->shiftDispersion->value() + ((src.width()-offset)/nrm - 10 + dsp + 0.5) * nrm,
                   src.height() - nrm / 2 - 1);
    }

    p.end();

    return src;
}

void SpectreDrawer::reDrawDispersion() {
    ui->drawer->spectre = drawDispersion(QImage());
    ui->drawer->repaint();
}

void SpectreDrawer::rotateMatrix() {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    if (v1 < 1e-6 || v2 < 1e-6) {
        v1 = 1;
        v2 = 1;
    }

    int dsp = ui->dispersion->value();
    int maxAt = 0;
    double maxRes = -1e+100;
    for (int i = 0; i < rawRes[0].size(); i++) {
        double cur = 0;
        for (int channel = 0; channel < rawRes.size(); channel++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dsp * channel / data.oneStep + 0.5);
            int v = (i + dt + rawRes[0].size() * 100) % rawRes[0].size();
            cur += rawRes[channel][v];
        }

        if (cur > maxRes) {
            maxAt = (i + int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dsp * (rawRes.size()) / data.oneStep + 0.5) + rawRes[0].size()*10000) % rawRes[0].size();
            maxRes = cur;
        }
    }

    int rotateCount = (rawRes[0].size() * 100000 + maxAt - 1) % rawRes[0].size();
    for (int channel = 0; channel < rawRes.size(); channel++)
        std::rotate(rawRes[channel].begin(), rawRes[channel].begin() + rotateCount, rawRes[channel].end());
}

void SpectreDrawer::saveAs() {
    QString savePath = QFileDialog::getSaveFileName(this, "Spectre", "", "Image *.png");
    if (savePath != "") {
        if (!savePath.endsWith(".png"))
            savePath += ".png";

        ui->drawer->spectre.save(savePath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
    }
}

void SpectreDrawer::mem() {
    QList<QVariant> lastSpectre;
    for (int j = 0; j < rawRes[0].size(); j++)
        for (int i = 0; i < rawRes.size(); i++)
            lastSpectre.push_back(rawRes[i][j]);

    QSettings().setValue("LastSpectre", lastSpectre);
}

void SpectreDrawer::memPlus() {
    addFromMem = true;
    reDraw();
}

QColor SpectreDrawer::jetModel(double value)
{
    QList<QColor> colors;
    QList<double> stops;
    if (colorModel == thermal) {
        colors = QList<QColor>() << QColor(0, 0, 50)
                                << QColor(20, 0, 120)
                                << QColor(200, 30, 140)
                                << QColor(255, 100, 0)
                                << QColor(255, 255, 40)
                                << QColor(255, 255, 255);
        stops = QList<double>() << 0 << 0.15 << 0.33 << 0.6 << 0.85 << 1;
    } else if (colorModel == grayScale) {
        value = 1.0 - value;
        colors = QList<QColor>() << QColor(0, 0, 0) << QColor(255, 255, 255);
        stops = QList<double>() << 0 << 1;
    } else if (colorModel == jet) {
        colors = QList<QColor>() << QColor(0, 0, 100)
                                 << QColor(0, 50, 255)
                                 << QColor(0, 255, 255)
                                 << QColor(255, 255, 0)
                                 << QColor(255, 30, 0)
                                 << QColor(100, 0, 0);
        stops = QList<double>() << 0 << 0.15 << 0.35 << 0.65 << 0.85 << 1;
    }
    if (value <= 0)
        return colors.first();

    for (int i = 0; i < stops.size(); i++)
        if (value <= stops.at(i))
            return interpolate(colors.at(i - 1), colors.at(i), (value - stops.at(i - 1)) / (stops.at(i) - stops.at(i - 1)));

    qWarning() << Q_FUNC_INFO << "something really wrong";
    return QColor::Invalid;
}

QColor SpectreDrawer::interpolate(QColor a, QColor b, double value)
{
    double rval = 1.0 - value;
    return QColor(a.red() * rval + b.red() * value,
                  a.green() * rval + b.green() * value,
                  a.blue() * rval + b.blue() * value);
}

void SpectreDrawer::changeModel(int newModel)
{
    colorModel = static_cast<ColorModels>(newModel);
    reDraw();
}
