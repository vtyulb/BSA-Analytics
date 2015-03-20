#include "analytics.h"
#include "ui_analytics.h"

#include <pulsarreader.h>
#include <pulsarworker.h>
#include <pulsarlist.h>
#include <settings.h>

#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSet>

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
    QObject::connect(ui->dispersionPlotButton, SIGNAL(clicked()), this, SLOT(dispersionPlot()));
    show();
    init();
}

void Analytics::init() {
    QSettings s;
    QString folder = QFileDialog::getExistingDirectory(this, "Path to *.pulsar files", s.value("openPath").toString());
    s.setValue("openPath", folder);

    loadPulsars(folder);
    pulsarsEnabled.resize(pulsars->size());
    window = new MainWindow(this);
    window->show();
    apply();
    ui->progressBar->hide();
    ui->currentFile->hide();
}

void Analytics::loadPulsars(QString dir) {
    qDebug() << "scanning directory" << dir;

    static int total = 0;

    QDir d(dir);
    QFileInfoList list = d.entryInfoList(QDir::Dirs);
    for (int i = 0; i < list.size(); i++)
        if (list[i].fileName() != "." && list[i].fileName() != "..")
            loadPulsars(list[i].absoluteFilePath());

    list = d.entryInfoList(QDir::Files);
    for (int i = 0; i < list.size(); i++) {
        ui->progressBar->setValue((i + 1) * 100 / list.size());
        qApp->processEvents();
        qDebug() << "reading file" << list[i].absoluteFilePath();
        ui->currentFile->setText(list[i].fileName());
        *pulsars += *PulsarReader::ReadPulsarFile(list[i].absoluteFilePath(), ui->progressBar);
        preCalc();
        if (Settings::settings()->lowMemory())
            for (static int j = 0; j < pulsars->size(); j++)
                (*pulsars)[j].squeeze();

        total++;
        ui->pulsarsTotal->setText(QString("Loaded %1 pulsar files").arg(total));
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

    if (ui->duplicatesCheckBox->isChecked())
        applyDuplicatesFilter();

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

bool Analytics::goodDoubles(double a, double b) {
    if (a > b)
        a /= b;
    else
        a = b / a;

    a = fabs(a - int(a + 0.5));

    return interval * a < 1.01 * b;
}

void Analytics::applyPeriodFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (goodDoubles(ui->period->value(), pulsars->at(i).period));
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

void Analytics::applyDuplicatesFilter() {
    const int duplicates = ui->duplicates->value();
    for (int i = 0; i < pulsars->size(); i++)
        (*pulsars)[i].firstPoint = 0;

    QSet<QString> *set = new QSet<QString>[pulsars->size()];

    for (int i = 0; i < pulsars->size(); i++)
        for (int j = i + 1; j < pulsars->size(); j++)
            if (pulsars->at(i).module == pulsars->at(j).module &&
                    pulsars->at(i).ray == pulsars->at(j).ray &&
                    pulsarsEnabled[i] && pulsarsEnabled[j] &&
                    pulsars->at(i).data.name != pulsars->at(j).data.name &&
                    goodDoubles(pulsars->at(i).period, pulsars->at(j).period) &&
                    !set[i].contains(pulsars->at(j).data.name)) {
                set[i].insert(pulsars->at(j).data.name);
                set[j].insert(pulsars->at(i).data.name);
                (*pulsars)[i].firstPoint++;
                (*pulsars)[j].firstPoint++;
//                qDebug() << i << j << "inc";
            }

    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).firstPoint >= duplicates);

    delete[] set;
}

void Analytics::preCalc() {
    for (static int i = 0; i < pulsars->size(); i++) {
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

void Analytics::dispersionPlot() {
    Data data;
    data.npoints = 200;
    data.modules = 1;
    data.rays = 1;
    data.channels = 1;
    data.time = pulsars->at(0).data.time;
    data.name = pulsars->at(0).data.name;
    data.init();


    double current = 0;
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < pulsars->size(); j++)
            if (pulsars->at(j).dispersion == i)
                current = pulsars->at(j).snr;

        data.data[0][0][0][i] = current;
    }

    window->regenerate(data);
}

Analytics::~Analytics()
{
    QSettings s;
    s.setValue("AnalyticsGeometry", this->saveGeometry());
    delete ui;
}
