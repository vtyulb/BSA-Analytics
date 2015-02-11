#include "analytics.h"
#include "ui_analytics.h"

#include <pulsarreader.h>
#include <pulsarworker.h>
#include <pulsarlist.h>

#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>

#include <algorithm>

Analytics::Analytics(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Analytics),
    pulsars(new QVector<Pulsar>),
    list(NULL)
{
    ui->setupUi(this);

    QSettings s;
    this->restoreGeometry(s.value("AnalyticsGeometry").toByteArray());

    QObject::connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));

    QString folder = QFileDialog::getExistingDirectory(this, "Path to *.pulsar files", s.value("openPath").toString());
    s.setValue("openPath", folder);
    show();

    loadPulsars(folder);
    pulsarsEnabled.resize(pulsars->size());
    preCalc();
    window = new MainWindow(this);
    window->show();
    apply();
}

void Analytics::loadPulsars(QString dir) {
    qDebug() << "scanning directory" << dir;

    QDir d(dir);
    QFileInfoList list = d.entryInfoList(QDir::Dirs);
    for (int i = 0; i < list.size(); i++)
        if (list[i].fileName() != "." && list[i].fileName() != "..")
            loadPulsars(list[i].absoluteFilePath());

    list = d.entryInfoList(QDir::Files);
    for (int i = 0; i < list.size(); i++) {
        ui->progressBar->setValue((i + 1) / list.size() * 100);
        update();
        qApp->processEvents();
        qDebug() << "reading file" << list[i].absoluteFilePath();
        *pulsars += *PulsarReader::ReadPulsarFile(list[i].absoluteFilePath());
    }
}

void Analytics::apply() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] = true;

    if (ui->moduleCheckBox->isChecked())
        applyModuleFilter();

    if (ui->periodCheckBox->isChecked())
        applyPeriodFilter();

    if (ui->SNRCheckBox->isChecked())
        applySNRFilter();

    if (ui->rayCheckBox->isChecked())
        applyRayFilter();

    if (ui->timeCheckBox->isChecked())
        applyTimeFilter();

    if (ui->multiplePicks->isChecked())
        applyMultiplePicksFilter();

    if (ui->strangeData->isChecked())
        applyStrangeDataFilter();

    if (ui->differentNoise->isChecked())
        applyDifferentNoise();

    Pulsars pl = new QVector<Pulsar>;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl->push_back(pulsars->at(i));

    if (!pl->size()) {
        QMessageBox::information(this, "Houston... We've Got a Problem", "There are no such pulsars");
        return;
    }

    std::sort(pl->data(), pl->data() + pl->size());

    delete list;
    list = new PulsarList("void", pl, this);
    list->show();
    QObject::connect(list, SIGNAL(switchData(Data&)), window, SLOT(regenerate(Data&)));
}

void Analytics::applyModuleFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).module == ui->module->value());
}

void Analytics::applyPeriodFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (PulsarWorker::goodDoubles(ui->period->value(), pulsars->at(i).period));
}

void Analytics::applyRayFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).ray == ui->ray->value());
}

void Analytics::applySNRFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).snr >= ui->SNR->value());
}

void Analytics::applyTimeFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (abs(pulsars->at(i).nativeTime.secsTo(ui->time->time())) < 120);
}

void Analytics::applyMultiplePicksFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (!pulsars->at(i).filtered);
}

void Analytics::applyStrangeDataFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i]) {
            const float *data = pulsars->at(i).data.data[0][0][0];
            float mx = 0;
            float mn = 0;
            for (int i = 0; data[i] != 0; i++) {
                if (data[i] > mx)
                    mx = data[i];

                if (data[i] < mn)
                    mn = data[i];
            }

            int res = 0;
            double step = (mx - mn) / 10;
            for (int i = 0; i < 10; i++) {
                bool rs = false;
                for (int j = 0; data[j] != 0; j++)
                    rs |= ((data[j] - mn >= i * step) && (data[j] - mn <= (i + 1) * step));

                res += rs;
            }

            if (res > 7)
                pulsarsEnabled[i] = false;
        }
}

void Analytics::applyDifferentNoise() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= differentNoisePreCalc[i];
}

void Analytics::preCalc() {
    qDebug() << "precalc";

    for (int i = 0; i < pulsars->size(); i++) {
        if (i % 1000 == 0) {
            ui->progressBar->setValue((i + 1) / pulsars->size() * 100);\
            qApp->processEvents();
        }

        int j = 0;
        while (pulsars->at(i).data.data[0][0][0][j] != 0) j++;
        while (pulsars->at(i).data.data[0][0][0][j] == 0) j++;

        QVector<double> sigmas;
        int n = pulsars->at(i).data.npoints - j;
        float *data = pulsars->at(i).data.data[0][0][0] + j;
        bool res = true;
        const int pieces = 8;
        for (int k = 0; k < pieces; k++) {
            double sigma = 0;
            for (int i = k * n / pieces; i < (k + 1) * n / pieces; i++)
                sigma += data[i] * data[i];

            sigma /= (n / pieces);
            sigma = pow(sigma, 0.5);
            sigmas.push_back(sigma);
        }

        for (int i = 0; i < pieces; i++)
            for (int j = 0; j < pieces; j++)
                if (sigmas[i] / sigmas[j] > 3)
                    res = false;

        differentNoisePreCalc.push_back(res);
    }
}

Analytics::~Analytics()
{
    QSettings s;
    s.setValue("AnalyticsGeometry", this->saveGeometry());
    delete ui;
}
