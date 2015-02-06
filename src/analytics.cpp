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

    Pulsars pl = new QVector<Pulsar>;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl->push_back(pulsars->at(i));

    if (!pl->size()) {
        QMessageBox::information(this, "Houston... We've Got a Problem", "There are no such pulsars");
        return;
    }

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

Analytics::~Analytics()
{
    QSettings s;
    s.setValue("AnalyticsGeometry", this->saveGeometry());
    delete ui;
}
