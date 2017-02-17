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

    QObject::connect(ui->binary, SIGNAL(clicked(bool)), ui->groupBox, SLOT(setDisabled(bool)));
    QObject::connect(ui->binary, SIGNAL(clicked(bool)), ui->dateTimeEdit, SLOT(setDisabled(bool)));
    QObject::connect(ui->binary, SIGNAL(clicked(bool)), ui->spinBox, SLOT(setDisabled(bool)));
    QObject::connect(ui->binary, SIGNAL(clicked(bool)), ui->checkBox, SLOT(setDisabled(bool)));

    QObject::connect(ui->text, SIGNAL(clicked(bool)), ui->groupBox, SLOT(setEnabled(bool)));
    QObject::connect(ui->text, SIGNAL(clicked(bool)), ui->dateTimeEdit, SLOT(setEnabled(bool)));
    QObject::connect(ui->text, SIGNAL(clicked(bool)), ui->spinBox, SLOT(setEnabled(bool)));
    QObject::connect(ui->text, SIGNAL(clicked(bool)), ui->checkBox, SLOT(setEnabled(bool)));


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

    ui->binary->click();
    resize(minimumSize());
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
    if (ui->lineEdit->text() != "") {
        int skip = ui->spinBox->value() * ui->checkBox->isChecked();
        emit customOpen(ui->lineEdit->text(), skip, ui->radioButton_2->isChecked() + ui->radioButton_3->isChecked() * 2, ui->dateTimeEdit->dateTime(), ui->binary->isChecked());
    }
    deleteLater();
}
