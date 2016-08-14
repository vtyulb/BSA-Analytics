#include "pulsarlist.h"
#include "ui_pulsarlist.h"
#include <pulsarreader.h>
#include <QDebug>
#include <QTimer>
#include <QSettings>

PulsarList::PulsarList(QString fileName, Pulsars pl, QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::PulsarList)
{
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

    ui->tableWidget->setRowCount(pulsars->size());
    ui->tableWidget->setColumnCount(6);
    QStringList header;
    header << "time" << "module" << "ray" << "dispersion" << "period" << "snr";
    ui->tableWidget->setHorizontalHeaderLabels(header);
    for (int i = 0; i < pulsars->size(); i++) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(getJName(pulsars->at(i).module, pulsars->at(i).ray, pulsars->at(i).nativeTime)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(pulsars->at(i).module)));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(pulsars->at(i).ray)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(pulsars->at(i).dispersion)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(pulsars->at(i).period, 'f', 5)));
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

QString PulsarList::getJName(int module, int ray, QTime time) {
    module--;
    ray--;
    static char *degree[6][8] = {
        {"4213", "4172", "4131", "4089", "4047", "4006", "3964", "3923"},
        {"3879", "3838", "3795", "3754", "3711", "3669", "3626", "3585"},
        {"3540", "3497", "3454", "3412", "3369", "3325", "3282", "3238"},
        {"3194", "3150", "3106", "3061", "3017", "2973", "2929", "2884"},
        {"2837", "2792", "2747", "2701", "2656", "2610", "2564", "2518"},
        {"2470", "2423", "2376", "2329", "2281", "2234", "2186", "2138"}
    };

    QString numb = degree[module][ray];
    int minutes = numb.right(2).toInt();
    minutes = minutes * 60 / 100;

    return "J" + time.toString("HHmm") + "+" + numb.left(2) + QString::number(minutes / 10) + QString::number(minutes % 10);
}

void PulsarList::closeEvent(QCloseEvent *) {
    QSettings().setValue("pulsar-list-geometry", saveGeometry());
}

PulsarList::~PulsarList() {
    delete ui;
}

void PulsarList::selectionChanged() {
    if (ui->tableWidget->selectionModel()->selection().indexes().size())
        if (ui->tableWidget->selectionModel()->selection().indexes().at(0).row() < pulsars->size())
            emit switchData((*pulsars)[ui->tableWidget->selectionModel()->selection().indexes().at(0).row()].data);
}
