#include "knownpulsarsgui.h"
#include "ui_knownpulsarsgui.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QDebug>
#include <QTableWidget>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

KnownPulsarsGUI::KnownPulsarsGUI(QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::KnownPulsarsGUI)
{
    QObject::setParent(parent);
    ui->setupUi(this);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    QObject::connect(ui->add, SIGNAL(clicked()), this, SLOT(add()));
    QObject::connect(ui->remove, SIGNAL(clicked()), this, SLOT(remove()));

    reload();
}

void KnownPulsarsGUI::reload() {
    pulsars.clear();
    QFile f(QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0] + KNOWN_PULSARS_FILENAME);
    if (f.open(QIODevice::ReadOnly)) {
        f.readLine();
        while (f.canReadLine()) {
            QByteArray line = f.readLine();
            if (line[0] == '#')
                continue;
            QTextStream stream(&line, QIODevice::ReadOnly);
            KnownPulsar pulsar;
            QString time;
            stream >> pulsar.module >> pulsar.ray >> pulsar.period >> time;
            if (time.size() < 6)
                break;

            pulsar.time = QTime::fromString(time, "hh:mm:ss");
            pulsars.push_back(pulsar);
        }

        f.remove();
        dump();
    }

    pulsars = load();

    ui->tableWidget->setRowCount(pulsars.size());
    ui->tableWidget->setColumnCount(4);

    QStringList l;
    l << "module" <<  "ray" << "period" << "time";
    ui->tableWidget->setHorizontalHeaderLabels(l);

    ui->tableWidget->setColumnWidth(0, 60);
    ui->tableWidget->setColumnWidth(1, 35);
    ui->tableWidget->setColumnWidth(2, 60);
    ui->tableWidget->setColumnWidth(3, width() - 180);

    for (int i = 0; i < pulsars.size(); i++) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(pulsars[i].module)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(pulsars[i].ray)));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(pulsars[i].period)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(pulsars[i].time.toString("HH:mm:ss")));
    }

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void KnownPulsarsGUI::add() {
    pulsars.push_back(KnownPulsar(ui->module->value(), ui->ray->value(), ui->period->value(), ui->timeEdit->time()));

    dump();
    reload();
}

void KnownPulsarsGUI::remove() {
    if (ui->tableWidget->selectionModel()->selection().indexes().size()) {
        int i = ui->tableWidget->selectionModel()->selection().indexes().at(0).row();
        if (i < pulsars.size()) {
            ui->module->setValue(pulsars[i].module);
            ui->ray->setValue(pulsars[i].ray);
            ui->period->setValue(pulsars[i].period);
            ui->timeEdit->setTime(pulsars[i].time);

            pulsars.remove(i);

            dump();
            reload();
        }
    }
}

void KnownPulsarsGUI::dump() {
    QList<QVariant> pulsarsList;
    for (int i = 0; i < pulsars.size(); i++)
        pulsarsList.push_back(pulsars[i].toVariant());

    QSettings().setValue("KnownPulsars", QVariant(pulsarsList));
}

QVector<KnownPulsar> KnownPulsarsGUI::load() {
    QVector<KnownPulsar> res;
    QList<QVariant> pulsarsList = QSettings().value("KnownPulsars").toList();
    for (int i = 0; i < pulsarsList.size(); i++)
        res.push_back(KnownPulsar(pulsarsList[i]));

    return res;
}

KnownPulsarsGUI::~KnownPulsarsGUI() {
    delete ui;
}
