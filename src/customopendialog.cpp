#include "customopendialog.h"
#include "ui_customopendialog.h"
#include <QString>
#include <QFileDialog>
#include <QButtonGroup>

CustomOpenDialog::CustomOpenDialog(QString lastOpenPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomOpenDialog),
    lastOpenPath(lastOpenPath)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openClicked()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(successFinish()));
    QObject::connect(this, SIGNAL(rejected()), this, SLOT(deleteLater()));

    QButtonGroup *group = new QButtonGroup(ui->groupBox);
    group->addButton(ui->radioButton);
    group->addButton(ui->radioButton_2);
    group->addButton(ui->radioButton_3);

    QButtonGroup *group2 = new QButtonGroup(ui->groupBox_2);
    group2->addButton(ui->binary);
    group2->addButton(ui->text);

    ui->text->setChecked(true);
    ui->radioButton_3->setChecked(true);
    ui->checkBox->setChecked(true);

    ui->dateTimeEdit->setMinimumDate(QDate(1900, 1, 1));
    ui->dateTimeEdit->setMaximumDate(QDate::currentDate());
    ui->dateTimeEdit->setDisplayFormat("dd.MM.yyyy hh:mm:ss");
}

CustomOpenDialog::~CustomOpenDialog()
{
    delete ui;
}

void CustomOpenDialog::openClicked() {
    QString path = QFileDialog::getOpenFileName(this, "", lastOpenPath);
    ui->lineEdit->setText(path);
}

void CustomOpenDialog::successFinish() {
    int skip = ui->spinBox->value() * ui->checkBox->isChecked();
    emit customOpen(ui->lineEdit->text(), skip, ui->radioButton_2->isChecked() + ui->radioButton_3->isChecked() * 2, ui->dateTimeEdit->dateTime(), ui->binary->isChecked());
    deleteLater();
}
