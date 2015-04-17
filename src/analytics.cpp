#include "analytics.h"
#include "ui_analytics.h"

#include <pulsarreader.h>
#include <pulsarworker.h>
#include <pulsarlist.h>
#include <settings.h>
#include <knownpulsarsgui.h>

#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSet>
#include <QTextBrowser>
#include <QStandardPaths>

#include <algorithm>

using std::max;

Analytics::Analytics(QString analyticsPath, QWidget *parent) :
    folder(analyticsPath),
    QWidget(parent),
    ui(new Ui::Analytics),
    pulsars(new QVector<Pulsar>),
    list(NULL),
    noises(new KnownNoise)
{
    ui->setupUi(this);

    QSettings s;
    this->restoreGeometry(s.value("AnalyticsGeometry").toByteArray());

    QObject::connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));
    QObject::connect(ui->dispersionPlotButton, SIGNAL(clicked()), this, SLOT(dispersionPlot()));
    QObject::connect(ui->addPulsarCatalog, SIGNAL(clicked()), this, SLOT(addPulsarCatalog()));
    QObject::connect(ui->infoButton, SIGNAL(clicked()),this, SLOT(showInfo()));
    QObject::connect(ui->knownPulsarsButton, SIGNAL(clicked()), this, SLOT(knownPulsarsGUI()));
    QObject::connect(ui->knownNoiseButton, SIGNAL(clicked()), noises, SLOT(show()));

    maxModule = 1;
    maxRay = 1;

    show();
    init();
}

void Analytics::init() {
    QSettings s;
    if (folder == "")
        folder = QFileDialog::getExistingDirectory(this, "Path to *.pulsar files", s.value("openPath").toString());

    s.setValue("openPath", folder);

    loadPulsars(folder);
    loadKnownPulsars();
    window = new MainWindow(this);
    window->show();
    apply();
    ui->progressBar->hide();
    ui->currentFile->hide();
}

void Analytics::loadKnownPulsars() {
    qDebug() << QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0];
    QFile f(QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0] + KNOWN_PULSARS_FILENAME);
    knownPulsars.clear();
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

            qDebug() << "loaded pulsar" << pulsar.module << pulsar.ray << pulsar.period << pulsar.time;
            knownPulsars.push_back(pulsar);
        }
    } else
        qDebug() << "can't find file" << f.fileName();
}

void Analytics::loadPulsars(QString dir) {
    qDebug() << "scanning directory" << dir;

    catalogs.push_back(dir);

    static int total = 0;


    QFileInfoList list;
    QDir d(dir);
    list = d.entryInfoList(QDir::Dirs);

    for (int i = 0; i < list.size(); i++)
        if (list[i].fileName() != "." && list[i].fileName() != "..")
            loadPulsars(list[i].absoluteFilePath());

    if (QFileInfo(dir).isDir())
        list = d.entryInfoList(QDir::Files);
    else
        list.push_back(QFileInfo(dir));

    for (int i = 0; i < list.size(); i++) {
        ui->progressBar->setValue((i + 1) * 100 / list.size());
        qApp->processEvents();
        qDebug() << "reading file" << list[i].absoluteFilePath();
        ui->currentFile->setText(list[i].fileName());
        *pulsars += *PulsarReader::ReadPulsarFile(list[i].absoluteFilePath(), ui->progressBar);
        preCalc();
        if (Settings::settings()->lowMemory())
            for (static int j = 0; j < pulsars->size(); j++)
                (*pulsars)[j].squeeze();

        total++;
        ui->pulsarsTotal->setText(QString("Loaded %1 pulsar files").arg(total));
    }

    pulsarsEnabled.resize(pulsars->size());
}

void Analytics::apply() {
    ui->progressBar->setValue(0);
    ui->progressBar->show();

    loadKnownPulsars();
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] = true;

    if (ui->moduleCheckBox->isChecked())
        applyModuleFilter();

    if (ui->periodCheckBox->isChecked())
        applyPeriodFilter();

    if (ui->SNRCheckBox->isChecked())
        applySNRFilter();

    if (ui->rayCheckBox->isChecked())
        applyRayFilter();

    if (ui->timeCheckBox->isChecked())
        applyTimeFilter();

    if (ui->multiplePicks->isChecked())
        applyMultiplePicksFilter();

    if (ui->knownPulsars->isChecked())
        applyKnownPulsarsFilter();

    if (ui->knownNoise->isChecked())
        applyKnownNoiseFilter();

    if (ui->strangeData->isChecked())
        applyStrangeDataFilter();

    if (ui->differentNoise->isChecked())
        applyDifferentNoise();

    if (ui->duplicates->value() > 1)
        for (int i = 0; i < ui->duplicatesIterations->value(); i++)
            applyDuplicatesFilter();


    Pulsars pl = new QVector<Pulsar>;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl->push_back(pulsars->at(i));

    if (!pl->size()) {
        QMessageBox::information(this, "Houston... We've Got a Problem", "There are no such pulsars");
        return;
    }

    std::sort(pl->data(), pl->data() + pl->size());

    delete list;
    list = new PulsarList("void", pl, this);
    list->show();
    QObject::connect(list, SIGNAL(switchData(Data&)), window, SLOT(regenerate(Data&)));

    ui->progressBar->hide();
}

void Analytics::applyModuleFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).module == ui->module->value());
}

bool Analytics::goodDoubles(double a, double b) {
    if (a < b) {
        double c = a;
        a = b;
        b = c;
    }

    if (a > 2.1 * b)
        return false;

    a /= b;

    a = fabs(a - int(a + 0.5));
    a = (a * interval / b);

    if (0.1 * b > 0.5)
        return a < 0.1 * b;
    else
        return a < 0.5;
}

void Analytics::applyPeriodFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (goodDoubles(ui->period->value(), pulsars->at(i).period));
}

void Analytics::applyRayFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).ray == ui->ray->value());
}

void Analytics::applySNRFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).snr >= ui->SNR->value());
}

void Analytics::applyTimeFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (abs(pulsars->at(i).nativeTime.secsTo(ui->time->time())) < 120);
}

void Analytics::applyMultiplePicksFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (!pulsars->at(i).filtered);
}

void Analytics::applyStrangeDataFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i]) {
            const float *data = pulsars->at(i).data.data[0][0][0];
            float mx = 0;
            float mn = 0;
            for (int i = 0; data[i] != 0; i++) {
                if (data[i] > mx)
                    mx = data[i];

                if (data[i] < mn)
                    mn = data[i];
            }

            int res = 0;
            double step = (mx - mn) / 10;
            for (int i = 0; i < 10; i++) {
                bool rs = false;
                for (int j = 0; data[j] != 0; j++)
                    rs |= ((data[j] - mn >= i * step) && (data[j] - mn <= (i + 1) * step));

                res += rs;
            }

            if (res > 7)
                pulsarsEnabled[i] = false;
        }
}

void Analytics::applyKnownPulsarsFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            for (int j = 0; j < knownPulsars.size(); j++)
                if (knownPulsars[j] == pulsars->at(i)) {
                    pulsarsEnabled[i] = false;
                    break;
                }
}

void Analytics::applyDifferentNoise() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= differentNoisePreCalc[i];
}

void Analytics::applyDuplicatesFilter() {
    const int duplicates = ui->duplicates->value();
    for (int i = 0; i < pulsars->size(); i++)
        (*pulsars)[i].firstPoint = 1;

    QSet<QString> *set = new QSet<QString>[pulsars->size()];

    QVector<int> pl[maxModule][maxRay];
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl[pulsars->at(i).module - 1][pulsars->at(i).ray - 1].push_back(i);

    for (int module = 0; module < 6; module++) {
        ui->progressBar->setValue(100 * module / 6);
        update();
        qApp->processEvents();

        for (int ray = 0; ray < 8; ray++)
            for (int k = 0; k < pl[module][ray].size(); k++)
                for (int l = k + 1; l < pl[module][ray].size(); l++) {
                    int i = pl[module][ray][k];
                    int j = pl[module][ray][l];

                    if (abs(pulsars->at(i).nativeTime.secsTo(pulsars->at(j).nativeTime)) < 120 &&
                            goodDoubles(pulsars->at(i).period, pulsars->at(j).period) &&
                            pulsars->at(i).data.name != pulsars->at(j).data.name &&
                            !set[i].contains(pulsars->at(j).data.name) &&
                            !set[j].contains(pulsars->at(i).data.name)) {
                        set[i].insert(pulsars->at(j).data.name);
                        set[j].insert(pulsars->at(i).data.name);
                        (*pulsars)[i].firstPoint++;
                        (*pulsars)[j].firstPoint++;
                    }
                }
    }

    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).firstPoint >= duplicates);

    delete[] set;
}

void Analytics::preCalc() {
    for (static int i = 0; i < pulsars->size(); i++) {
        maxModule = max(maxModule, pulsars->at(i).module);
        maxRay = max(maxRay, pulsars->at(i).ray);

        differentNoisePreCalc.push_back(!pulsars->at(i).badNoise());
    }
}

void Analytics::dispersionPlot() {
    Data data;
    data.npoints = 200;
    data.modules = 1;
    data.rays = 1;
    data.channels = 1;
    data.time = pulsars->at(0).data.time;
    data.name = pulsars->at(0).data.name;
    data.init();


    for (int i = 0; i < 200; i++) {
        double current = 0;
        for (int j = 0; j < pulsars->size(); j++)
            if (pulsarsEnabled[j])
                if (pulsars->at(j).dispersion == i)
                    current = pulsars->at(j).snr;

        data.data[0][0][0][i] = current;
    }

    window->regenerate(data);
}

void Analytics::addPulsarCatalog() {
    QString catalog = QFileDialog::getExistingDirectory(this, "pulsar catalog", QSettings().value("openPath").toString());
    if (catalog != "") {
        ui->progressBar->show();
        ui->currentFile->show();
        loadPulsars(catalog);
        ui->currentFile->hide();
        ui->progressBar->hide();
    }
}

void Analytics::showInfo() {
    QString data = "Information about current instance<br/>";
    data += "Known pulsars:<br/>";
    data += "module ray period time<br/>";
    for (int i = 0; i < knownPulsars.size(); i++)
        data += QString("%1 %2 %3 %4<br/>").arg(QString::number(knownPulsars[i].module),
                                                QString::number(knownPulsars[i].ray),
                                                QString::number(knownPulsars[i].period),
                                                knownPulsars[i].time.toString("HH:mm:ss"));

    data += "<br/><br/>Loaded catalogs:<br/>";
    for (int i = 0; i < catalogs.size(); i++)
        data += catalogs[i] + "<br/>";

    QTextBrowser *browser = new QTextBrowser();
    browser->setHtml(data);
    browser->setWindowTitle("Info");
    browser->show();
}

void Analytics::knownPulsarsGUI() {
    static KnownPulsarsGUI *gui = new KnownPulsarsGUI();
    gui->show();
}

void Analytics::applyKnownNoiseFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= !noises->contains(pulsars->at(i).period);
}

Analytics::~Analytics()
{
    QSettings s;
    s.setValue("AnalyticsGeometry", this->saveGeometry());
    delete ui;
}
