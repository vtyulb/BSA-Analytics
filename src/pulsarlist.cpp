#include "pulsarlist.h"
#include "ui_pulsarlist.h"

#include <pulsarreader.h>

#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QTableWidget>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

PulsarList::PulsarList(QString fileName, Pulsars pl, bool removeBadData, QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::PulsarList)
{
    QObject::setParent(parent);
    if (pl == NULL)
        pulsars = PulsarReader::ReadPulsarFile(fileName);
    else
        pulsars = pl;

    if (!pulsars->size()) {
        deleteLater();
        return;
    }

    ui->setupUi(this);

    QObject::connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    QObject::connect(ui->tableWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));

    QAction *showUTCtime = new QAction("Show UTC time", this);
    QObject::connect(showUTCtime, SIGNAL(triggered(bool)), this, SLOT(showTime()));
    ui->tableWidget->addAction(showUTCtime);
    ui->tableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

    ui->tableWidget->setRowCount(pulsars->size());
    ui->tableWidget->setColumnCount(6);
    QStringList header;
    header << "time" << "module" << "ray" << "dispersion" << "period" << "snr";
    ui->tableWidget->setHorizontalHeaderLabels(header);
    for (int i = 0; i < pulsars->size(); i++) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(getPulsarJName(pulsars->at(i).module, pulsars->at(i).ray, pulsars->at(i).nativeTime)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(pulsars->at(i).module)));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(pulsars->at(i).ray)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(pulsars->at(i).dispersion)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(pulsars->at(i).period, 'f', 5)));
        ui->tableWidget->setItem(i, 5, new QTableWidgetItem(QString::number(pulsars->at(i).snr, 'f', 1)));

        if (pulsars->at(i).filtered)
            for (int j = 0; j < ui->tableWidget->columnCount(); j++)
                ui->tableWidget->item(i, j)->setBackgroundColor(QColor("lightgray"));

        if (pulsars->at(i).dispersion == -7777)
            for (int j = 0; j < ui->tableWidget->columnCount(); j++)
                ui->tableWidget->item(i, j)->setBackgroundColor(QColor(200, 100, 100));

        if (pulsars->at(i).fourierDuplicate)
            ui->tableWidget->item(i, 0)->setBackgroundColor(QColor(255, 255, 150));

        pulsarsIndex.push_back(i);
    }

    if (removeBadData) {
        int v = 0;
        for (int i = 0; i < pulsars->size(); i++)
            if (!(pulsars->at(i).dispersion == -7777)) {
                for (int j = 0; j < ui->tableWidget->columnCount(); j++)
                    ui->tableWidget->setItem(v, j, new QTableWidgetItem(*(ui->tableWidget->item(i, j))));

                pulsarsIndex[v] = i;
                v++;
            }

        ui->tableWidget->setRowCount(v);
    }

    ui->tableWidget->setStyleSheet("QMenu::item:selected{border:1px solid red;}");

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
    qApp->quit();
}

PulsarList::~PulsarList() {
    delete ui;
}

void PulsarList::selectionChanged() {
    if (ui->tableWidget->selectionModel()->selection().indexes().size())
        if (ui->tableWidget->selectionModel()->selection().indexes().at(0).row() < pulsars->size()) {
            currentPulsar = &(*pulsars)[pulsarsIndex[ui->tableWidget->selectionModel()->selection().indexes().at(0).row()]];
            emit switchData(currentPulsar->data);
        }
}

void PulsarList::showTime() {
    QMessageBox::information(this, "Peak UTC time", currentPulsar->UTCtime());
}
