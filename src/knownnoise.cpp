#include "knownnoise.h"
#include "ui_knownnoise.h"

#include <analytics.h>

#include <QSettings>


KnownNoise::KnownNoise() :
    QWidget(NULL),
    ui(new Ui::KnownNoise)
{
    ui->setupUi(this);

    reload();

    QObject::connect(ui->add, SIGNAL(clicked()), this, SLOT(add()));
    QObject::connect(ui->remove, SIGNAL(clicked()), this, SLOT(remove()));
}

KnownNoise::~KnownNoise()
{
    delete ui;
}

void KnownNoise::reload() {
    doubles.clear();

    QVector<QVariant> v = QSettings().value("knownNoises").toList().toVector();
    for (int i = 0; i < v.size(); i++)
        doubles.push_back(v[i].toDouble());

    ui->tableWidget->setHorizontalHeaderLabels(QStringList("Period"));
    ui->tableWidget->setColumnCount(1);
    ui->tableWidget->setRowCount(doubles.size());
    for (int i = 0; i < doubles.size(); i++)
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(doubles[i])));

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void KnownNoise::save() {
    QVector<QVariant> v;
    for (int i = 0; i < doubles.size(); i++)
        v.push_back(doubles[i]);

    QSettings().setValue("knownNoises", v.toList());
}

void KnownNoise::remove() {
    if (ui->tableWidget->selectionModel()->selection().indexes().size()) {
        int i = ui->tableWidget->selectionModel()->selection().indexes().at(0).row();
        ui->period->setValue(doubles[i]);
        doubles.remove(i);
        save();
        reload();
    }
}

bool KnownNoise::contains(double p) {
    for (int i = 0; i < doubles.size(); i++)
        if (globalGoodDoubles(p, doubles[i]))
            return true;

    return false;
}

void KnownNoise::add() {
    doubles.push_back(ui->period->value());
    save();
    reload();
}
