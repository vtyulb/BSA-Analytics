#include "customopendialog.h"
#include "ui_customopendialog.h"
#include <QString>
#include <QFileDialog>
#include <QButtonGroup>

CustomOpenDialog::CustomOpenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomOpenDialog)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openClicked()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(successFinish()));

    QButtonGroup *group = new QButtonGroup(ui->groupBox);
    group->addButton(ui->radioButton);
    group->addButton(ui->radioButton_2);
    group->addButton(ui->radioButton_3);

    ui->radioButton_3->setChecked(true);
    ui->checkBox->setChecked(true);
}

CustomOpenDialog::~CustomOpenDialog()
{
    delete ui;
}

void CustomOpenDialog::openClicked() {
    QString path = QFileDialog::getOpenFileName(this);
    ui->lineEdit->setText(path);
}

void CustomOpenDialog::successFinish() {
    int skip = ui->spinBox->value() * ui->checkBox->isChecked();
    emit customOpen(ui->lineEdit->text(), skip, ui->radioButton_2->isChecked() + ui->radioButton_3->isChecked() * 2);
}
