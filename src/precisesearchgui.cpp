#include "precisesearchgui.h"
#include "ui_precisesearchgui.h"

#include <QFileDialog>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QDebug>

PreciseSearchGui::PreciseSearchGui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreciseSearchGui)
{
    ui->setupUi(this);

    QObject::connect(ui->selectButton, SIGNAL(clicked()), this, SLOT(selectFile()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(runSearcher()));
}

PreciseSearchGui::~PreciseSearchGui()
{
    delete ui;
}

void PreciseSearchGui::selectFile() {
    ui->fileName->setText(QFileDialog::getOpenFileName(this, "Binary data file", QSettings().value("openPath").toString()));
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

    qDebug() << "running with" << l;
    QProcess::startDetached(qApp->arguments().at(0), l);
}
