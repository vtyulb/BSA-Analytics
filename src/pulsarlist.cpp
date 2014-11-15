#include "pulsarlist.h"
#include "ui_pulsarlist.h"
#include <pulsarreader.h>

PulsarList::PulsarList(QString fileName, QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::PulsarList),
    pulsars(PulsarReader::ReadPulsarFile(fileName))
{
    if (!pulsars->size()) {
        deleteLater();
        return;
    }

    ui->setupUi(this);

    QObject::connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    QObject::connect(ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(selectionChanged(int, int, int, int)));

    ui->tableWidget->setRowCount(pulsars->size());
    ui->tableWidget->setColumnCount(6);
    QStringList header;
    header << "time" << "module" << "ray" << "dispersion" << "period" << "snr";
    ui->tableWidget->setHorizontalHeaderLabels(header);
    for (int i = 0; i < pulsars->size(); i++) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(pulsars->at(i).nativeTime.toString("hh:mm:ss")));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(pulsars->at(i).module)));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(pulsars->at(i).ray)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(pulsars->at(i).dispersion)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(pulsars->at(i).period, 'f', 3)));
        ui->tableWidget->setItem(i, 5, new QTableWidgetItem(QString::number(pulsars->at(i).snr, 'f', 1)));
    }

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int i = 0; i < ui->tableWidget->columnCount(); i++)
        ui->tableWidget->setColumnWidth(i, 65);

    ui->tableWidget->setColumnWidth(2, 30);


    show();
}

PulsarList::~PulsarList()
{
    delete ui;
    delete pulsars;
}

void PulsarList::selectionChanged(int x1, int y1, int x2, int y2) {
    emit switchData((*pulsars)[y1].data);
}
