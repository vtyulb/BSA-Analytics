#include "precisesearchgui.h"
#include "ui_precisesearchgui.h"
#include "ui_precisepacket.h"

#include <QFileDialog>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include <QButtonGroup>
#include <QRadioButton>
#include <QSpinBox>
#include <QThread>
#include <QLabel>

PreciseSearchGui::PreciseSearchGui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreciseSearchGui)
{
    ui->setupUi(this);

    QObject::connect(ui->selectButton, SIGNAL(clicked()), this, SLOT(selectFile()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(runSearcher()));

    QObject::connect(ui->preciseSearch, SIGNAL(clicked(bool)), this, SLOT(preciseSearchMode()));
    QObject::connect(ui->spectre, SIGNAL(clicked(bool)), this, SLOT(nonPreciseSearchMode()));
    QObject::connect(ui->singlePeriod, SIGNAL(clicked(bool)), this, SLOT(nonPreciseSearchMode()));

    ui->threadCount->setMaximum(QThread::idealThreadCount());

    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(ui->preciseSearch);
    group->addButton(ui->spectre);
    group->addButton(ui->singlePeriod);

    resize(minimumSize());
}

PreciseSearchGui::~PreciseSearchGui()
{
    delete ui;
}

void PreciseSearchGui::selectFile() {
    ui->fileName->setText(QFileDialog::getOpenFileName(this, "Binary data file", QSettings().value("openPath").toString()));
    if (ui->fileName->text().contains(".precise-packet")) {
        QDialog dialog;

        Ui::Dialog ui2;
        ui2.setupUi(&dialog);
        if (dialog.exec()) {
            runPacketSearcher();
            reject();
        }
    }

}

void PreciseSearchGui::runPacketSearcher() {
    QStringList l;
    l << "--precise-packet" << ui->fileName->text();
    QProcess::startDetached(qApp->arguments().at(0), l);
}

void PreciseSearchGui::runSearcher() {
    QStringList l;
    l << "--precise-pulsar-search";
    l << ui->fileName->text();

    l << "--module" << QString::number(ui->module->value());
    l << "--ray" << QString::number(ui->ray->value());
    l << "--period" << QString::number(ui->period->value());
    l << "--dispersion" << QString::number(ui->dispersion->value());
    l << "--time" << ui->time->time().toString("HH:mm:ss");
    if (ui->skipMultiplePeriods->isChecked())
        l << "--no-multiple-periods";

    if (ui->runAnalytics->isEnabled() && ui->runAnalytics->isChecked())
        l << "--run-analytics-after";

    if (!ui->clearNoise->isChecked())
        l << "--do-not-clear-noise";

    if (ui->singlePeriod->isChecked())
        l << "--single-period";

    if (ui->spectre->isChecked())
        l << "--draw-spectre";

    l << "--threads" << QString::number(ui->threadCount->value());

    qDebug() << "running with" << l;
    QProcess::startDetached(qApp->arguments().at(0), l);
}

void PreciseSearchGui::preciseSearchMode() {
    ui->skipMultiplePeriods->setEnabled(true);
    ui->skipPeriodsLabel->setEnabled(true);
    ui->runAnalytics->setEnabled(true);
}

void PreciseSearchGui::nonPreciseSearchMode() {
    ui->skipMultiplePeriods->setEnabled(false);
    ui->skipPeriodsLabel->setEnabled(false);

    if (ui->spectre->isChecked())
        ui->runAnalytics->setEnabled(false);
    else
        ui->runAnalytics->setEnabled(true);
}
