#include "pulsarlist.h"
#include "ui_pulsarlist.h"
#include <pulsarreader.h>
#include <QDebug>
#include <QTimer>
#include <QSettings>

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
    QObject::connect(ui->tableWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));

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

        if (pulsars->at(i).filtered)
            for (int j = 0; j < ui->tableWidget->columnCount(); j++)
                ui->tableWidget->item(i, j)->setBackgroundColor(QColor("lightgray"));
    }

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int i = 0; i < ui->tableWidget->columnCount(); i++)
        ui->tableWidget->setColumnWidth(i, 68);

    ui->tableWidget->setColumnWidth(2, 30);
    ui->tableWidget->setColumnWidth(3, 80);
    ui->tableWidget->setColumnWidth(5, 50);
    ui->tableWidget->selectRow(0);

    QTimer::singleShot(200, this, SLOT(selectionChanged()));

    restoreGeometry(QSettings().value("pulsar-list-geometry").toByteArray());
    show();
}

void PulsarList::closeEvent(QCloseEvent *) {
    QSettings().setValue("pulsar-list-geometry", saveGeometry());
}

PulsarList::~PulsarList() {
    delete ui;
    delete pulsars;
}

void PulsarList::selectionChanged() {
    emit switchData((*pulsars)[ui->tableWidget->selectionModel()->selection().indexes().at(0).row()].data);
}
