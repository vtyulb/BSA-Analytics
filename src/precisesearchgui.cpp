#include "precisesearchgui.h"
#include "ui_precisesearchgui.h"
#include "ui_precisepacket.h"
#include <flowdetecter.h>
#include <spectredrawer.h>
#include <settings.h>

#include <QApplication>
#include <QFileDialog>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include <QButtonGroup>
#include <QRadioButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QThread>
#include <QLabel>
#include <QMessageBox>

PreciseSearchGui::PreciseSearchGui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreciseSearchGui)
{
    ui->setupUi(this);

    QObject::connect(ui->selectButton, SIGNAL(clicked()), this, SLOT(selectFile()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(runSearcher()));

    QObject::connect(ui->preciseSearch, SIGNAL(clicked(bool)), this, SLOT(determineSearchMode()));
    QObject::connect(ui->fluxDensity, SIGNAL(clicked(bool)), this, SLOT(determineSearchMode()));
    QObject::connect(ui->spectre, SIGNAL(clicked(bool)), this, SLOT(determineSearchMode()));
    QObject::connect(ui->singlePeriod, SIGNAL(clicked(bool)), this, SLOT(determineSearchMode()));

    ui->threadCount->setMaximum(QThread::idealThreadCount());

    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(ui->preciseSearch);
    group->addButton(ui->spectre);
    group->addButton(ui->singlePeriod);
    group->addButton(ui->fluxDensity);

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
    if (ui->fileName->text() == "")
        return;

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

    if (ui->normalize->isChecked())
        l << "--normalize";

    if (ui->runAnalytics->isEnabled() && ui->runAnalytics->isChecked())
        l << "--run-analytics-after";

    if (!ui->clearNoise->isChecked())
        l << "--do-not-clear-noise";

    if (ui->singlePeriod->isChecked())
        l << "--single-period";

    if (ui->spectre->isChecked()) {
        SpectreDrawer *drawer = new SpectreDrawer;
        Settings::settings()->setDispersion(ui->dispersion->value());
        drawer->drawSpectre(ui->module->value() - 1, ui->ray->value() - 1, ui->fileName->text(), ui->time->time(), ui->period->value());
        return;
    }

    if (qApp->arguments().contains("--debug"))
        l << "--debug";

    if (ui->fluxDensity->isChecked()) {
        FlowDetecter detecter(ui->module->value(), ui->dispersion->value(), ui->ray->value(),
                              ui->points->value(), ui->bigImpulses->isChecked(), ui->sensitivity->value(),
                              ui->period->value(), ui->time->time(), ui->fileName->text(), this);
        detecter.run();
        return;
    }

    l << "--threads" << QString::number(ui->threadCount->value());

    qDebug() << "running with" << l;
    QProcess::startDetached(qApp->arguments().at(0), l);
}

void PreciseSearchGui::determineSearchMode() {
    ui->skipMultiplePeriods->setEnabled(true);
    ui->skipMultiplePeriodsLabel->setEnabled(true);
    ui->runAnalytics->setEnabled(true);
    ui->runAnalyticsAfterLabel->setEnabled(true);
    ui->threadCount->setEnabled(true);
    ui->threadCountLabel->setEnabled(true);
    ui->fluxDensityWidget->setEnabled(false);
    ui->dispersion->setDecimals(0);
    ui->normalize->setEnabled(true);
    ui->normalizeLabel->setEnabled(true);
    ui->clearNoise->setEnabled(true);
    ui->clearNoiseLabel->setEnabled(true);

    if (ui->singlePeriod->isChecked()) {
        ui->skipMultiplePeriods->setEnabled(false);
        ui->skipMultiplePeriodsLabel->setEnabled(false);
        ui->dispersion->setDecimals(1);
    }

    if (ui->spectre->isChecked()) {
        ui->runAnalytics->setEnabled(false);
        ui->runAnalyticsAfterLabel->setEnabled(false);
        ui->skipMultiplePeriods->setEnabled(false);
        ui->skipMultiplePeriodsLabel->setEnabled(false);
        ui->clearNoise->setEnabled(false);
        ui->clearNoiseLabel->setEnabled(false);
        ui->threadCount->setEnabled(false);
        ui->threadCountLabel->setEnabled(false);
        ui->normalize->setEnabled(false);
        ui->normalizeLabel->setEnabled(false);
    }

    if (ui->fluxDensity->isChecked()) {
        if (!QSettings().contains("LongStair") || !QSettings().contains("ShortStair")) {
            QMessageBox::warning(this, "We need more stairs!", "Please do set stairs in both long and short data!\n"
                                                               "I can't run flux density without stairs.");
            ui->preciseSearch->click();
            return;
        }
        ui->fluxDensityWidget->setEnabled(true);
        ui->skipMultiplePeriods->setEnabled(false);
        ui->skipMultiplePeriodsLabel->setEnabled(false);
        ui->clearNoise->setEnabled(false);
        ui->clearNoiseLabel->setEnabled(false);
        ui->runAnalytics->setEnabled(false);
        ui->runAnalyticsAfterLabel->setEnabled(false);
        ui->threadCount->setEnabled(false);
        ui->threadCountLabel->setEnabled(false);
        ui->normalize->setEnabled(false);
        ui->normalizeLabel->setEnabled(false);
    }

    ui->points->setEnabled(false);
    ui->pointsLabel->setEnabled(false);
}
