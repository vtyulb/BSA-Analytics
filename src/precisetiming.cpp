#include "precisetiming.h"
#include "ui_precisetiming.h"

#include <QFileDialog>
#include <QProcess>
#include <QStringList>

PreciseTiming::PreciseTiming(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreciseTiming)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton_1, SIGNAL(clicked(bool)), this, SLOT(setFile1()));
    QObject::connect(ui->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(setFile2()));
    QObject::connect(ui->pushButton_3, SIGNAL(clicked(bool)), this, SLOT(setFile3()));

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(run()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

PreciseTiming::~PreciseTiming()
{
    delete ui;
}

void PreciseTiming::setFile1() {
    QString name = QFileDialog::getOpenFileName(this, "*.pnt file");
    if (name != "")
        ui->lineEdit_1->setText(name);
}

void PreciseTiming::setFile2() {
    QString name = QFileDialog::getOpenFileName(this, "*.pnt file");
    if (name != "")
        ui->lineEdit_2->setText(name);
}

void PreciseTiming::setFile3() {
    QString name = QFileDialog::getOpenFileName(this, "*.pnt file");
    if (name != "")
        ui->lineEdit_3->setText(name);
}

void PreciseTiming::run() {
    QStringList l;

    l << "--precise-timing";
    l << ui->lineEdit_1->text();
    l << ui->lineEdit_2->text();
    l << ui->lineEdit_3->text();

    l << "--dispersion" << QString::number(ui->dispersion->value());
    l << "--period" << QString::number(ui->period->value());
    l << "--module" << QString::number(ui->module->value());
    l << "--ray" << QString::number(ui->ray->value());
    l << "--time" << ui->time->time().toString("HH:mm:ss");

    QProcess::startDetached(qApp->arguments().at(0), l);
    close();
}
