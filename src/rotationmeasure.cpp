#include "rotationmeasure.h"
#include "ui_rotationmeasure.h"

#include <QFileDialog>
#include <QMessageBox>

#include <reader.h>
#include <settings.h>

RotationMeasure::RotationMeasure(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RotationMeasure)
{
    ui->setupUi(this);

    QObject::connect(ui->fileButton, SIGNAL(clicked(bool)), this, SLOT(setFile()));
    QObject::connect(ui->stairButton, SIGNAL(clicked(bool)), this, SLOT(setStairFile()));
    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(run()));
}

void RotationMeasure::setFile() {
    ui->filePath->setText(QFileDialog::getOpenFileName(this));
}

void RotationMeasure::setStairFile() {
    ui->stairPath->setText(QFileDialog::getOpenFileName(this));
}

void RotationMeasure::run() {
    QVector<double> res;

    int module = ui->module->value() - 1;
    int ray = ui->ray->value() - 1;

    Reader r1;
    QObject::connect(&r1, SIGNAL(progress(int)), Settings::settings()->getProgressBar(), SLOT(setValue(int)));
    Data stair = r1.readBinaryFile(ui->stairPath->text());
    QVector<double> stairSizes;
    for (int i = 0; i < stair.channels - 1; i++)
        stairSizes.push_back(stair.stairHeight(module, ray, i));

    stair.releaseData();


    Reader r2;
    QObject::connect(&r2, SIGNAL(progress(int)), Settings::settings()->getProgressBar(), SLOT(setValue(int)));
    Data data = r2.readBinaryFile(ui->filePath->text());

    for (int i = 0; i < data.channels - 1; i++) {
        double value = data.data[module][i][ray][ui->top->value()] -
                        data.data[module][i][ray][ui->bottom->value()];

        qDebug() << "module" << module << "ray" << ray
                 << "channel" << i << "top" << data.data[module][i][ray][ui->top->value()]
                 << "bottom" << data.data[module][i][ray][ui->bottom->value()];

        value /= stairSizes[i];

        res.push_back(value);
    }

    QString resStr;
    for (int i = 0; i < data.channels - 1; i++) {
        resStr += QString::number(res[i]) + "\n";
    }

    QMessageBox::information(this, "source height", resStr);

    stair.releaseData();
    data.releaseData();
}

RotationMeasure::~RotationMeasure()
{
    delete ui;
}
