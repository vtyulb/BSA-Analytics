#include "analytics.h"
#include "ui_analytics.h"

#include <pulsarreader.h>
#include <pulsarworker.h>
#include <pulsarlist.h>
#include <settings.h>
#include <knownpulsarsgui.h>
#include <fourier.h>

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

Analytics::Analytics(QString analyticsPath, bool fourier, QWidget *parent) :
    folder(analyticsPath),
    fourier(fourier),
    QWidget(parent),
    ui(new Ui::Analytics),
    pulsars(new QVector<Pulsar>),
    list(NULL),
    noises(new KnownNoise)
{
    ui->setupUi(this);

    QObject::connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));
    QObject::connect(ui->addPulsarCatalog, SIGNAL(clicked()), this, SLOT(addPulsarCatalog()));
    QObject::connect(ui->infoButton, SIGNAL(clicked()),this, SLOT(showInfo()));
    QObject::connect(ui->knownPulsarsButton, SIGNAL(clicked()), this, SLOT(knownPulsarsGUI()));
    QObject::connect(ui->knownNoiseButton, SIGNAL(clicked()), noises, SLOT(show()));

    QObject::connect(ui->dispersionPlotButton, SIGNAL(clicked()), this, SLOT(dispersionPlot()));
    QObject::connect(ui->dispersionM, SIGNAL(clicked()), this, SLOT(dispersionMplus()));
    QObject::connect(ui->dispersionRemember, SIGNAL(clicked()), this, SLOT(dispersionRemember()));

    QObject::connect(ui->fourierBlockNo, SIGNAL(valueChanged(int)), this, SLOT(actualFourierDataChanged()));
    QObject::connect(ui->fourierLoad, SIGNAL(clicked(bool)), this, SLOT(loadFourierData()));
    QObject::connect(ui->fourierCalculateCashes, SIGNAL(clicked(bool)), this, SLOT(calculateCashes()));

    maxModule = 1;
    maxRay = 1;
    fourierSpectreSize = 1024;

    fileNames.push_back("all files");

    if (fourier) {
        Settings::settings()->setFourierAnalytics(true);
        ui->groupBox_2->hide();
        ui->groupBox->hide();
        ui->groupBox_4->hide();
        ui->fileNames->hide();

        fourierData.resize(500);
    } else {
        ui->groupBox_5->hide();
        ui->groupBox_6->hide();
    }

    QSettings s;
    this->restoreGeometry(s.value("AnalyticsGeometry").toByteArray());

    show();
    init();
}

void Analytics::init() {
    QSettings s;
    if (folder == "")
        folder = QFileDialog::getExistingDirectory(this, "Path to *.pulsar files", s.value("openPath").toString());

    s.setValue("openPath", folder);

    window = new MainWindow(this);
    window->show();
    loadPulsars(folder);
    loadKnownPulsars();
    if (!fourier)
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

    if (fourier) {
        loadFourierData();
        return;
    }

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
        ui->currentFile->setText(list[i].fileName());
        Pulsars p = PulsarReader::ReadPulsarFile(list[i].absoluteFilePath(), ui->progressBar);
        *pulsars += (*p);
        delete p;

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
    applyFourierFilters();
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] = true;

    if (ui->periodRangeCheckBox->isChecked())
        applyPeriodRangeFilter();

    if (ui->moduleCheckBox->isChecked())
        applyModuleFilter();

    if (ui->periodCheckBox->isChecked())
        applyPeriodFilter();

    if (ui->rayCheckBox->isChecked())
        applyRayFilter();

    if (ui->timeCheckBox->isChecked())
        applyTimeFilter();

    if (ui->multiplePicks->isChecked())
        applyMultiplePicksFilter();

    if (ui->knownPulsars->isChecked() && !fourier)
        applyKnownPulsarsFilter();

    if (ui->knownNoise->isChecked() && !fourier)
        applyKnownNoiseFilter();

    if (ui->differentNoise->isChecked())
        applyDifferentNoise();

    if (ui->strangeData->isChecked())
        applyStrangeDataFilter();

    if (ui->fileNames->currentIndex() != 0)
        applyFileNameFilter();

    if (ui->duplicates->value() > 1)
        for (int i = 0; i < ui->duplicatesIterations->value(); i++)
            applyDuplicatesFilter();

    if (ui->SNRCheckBox->isChecked())
        applySNRFilter();

    if (ui->differentMaximums->isChecked())
        applyDifferentMaximumsFilter();

    Pulsars pl = new QVector<Pulsar>;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl->push_back(pulsars->at(i));

    if (!pl->size()) {
        QMessageBox::information(this, "Houston... We've Got a Problem", "There are no such pulsars");
        return;
    }

    if (!fourier)
        std::sort(pl->data(), pl->data() + pl->size());

    delete list;
    list = new PulsarList("void", pl, ui->fourierRemoveBadRawData->isChecked(), this);
    list->show();
    QObject::connect(list, SIGNAL(switchData(Data&)), window, SLOT(regenerate(Data&)));

    ui->progressBar->hide();
}

void Analytics::applyFileNameFilter() {
    if (fourier)
        return;

    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pulsarsEnabled[i] &= (pulsars->at(i).data.name == fileNames[ui->fileNames->currentIndex()]);
}

void Analytics::applyModuleFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (pulsars->at(i).module == ui->module->value());
}

void Analytics::applyPeriodRangeFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (ui->periodRangeLeft->value() < pulsars->at(i).period) &&
                                (ui->periodRangeRight->value() > pulsars->at(i).period);
}

void Analytics::applyPeriodFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= (globalGoodDoubles(ui->period->value(), pulsars->at(i).period, ui->doublePeriods->isChecked()));
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

void Analytics::applyDifferentMaximumsFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i]) {
            double mx1 = 0;
            double mx2 = 0;
            int sz;
            float *data = pulsars->at(i).data.data[0][0][0];
            for (sz = 0; data[sz] != 0; sz++);

            for (int j = 0; j < sz / 2; j++)
                if (data[j] > mx1)
                    mx1 = data[j];

            for (int j = sz / 2; j < sz; j++)
                if (data[j] > mx2)
                    mx2 = data[j];


            if (mx1 > mx2)
                mx1 /= mx2;
            else
                mx1 = mx2 / mx1;

            pulsarsEnabled[i] &= mx1 < 1.4;
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
                            globalGoodDoubles(pulsars->at(i).period, pulsars->at(j).period, ui->doublePeriods->isChecked()) &&
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

        if (!fileNames.contains(pulsars->at(i).data.name))
            fileNames.push_back(pulsars->at(i).data.name);

        differentNoisePreCalc.push_back(!pulsars->at(i).badNoise());
    }

    ui->fileNames->clear();
    for (int i = 0; i < fileNames.size(); i++)
        ui->fileNames->addItem(fileNames[i]);
}

Data Analytics::dispersionGenerateData() {
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

    return data;
}

void Analytics::dispersionPlot() {
    Data data = dispersionGenerateData();
    window->regenerate(data);
}

void Analytics::dispersionRemember() {
    Data data = Settings::settings()->lastData();
    QVector<double> dt;
    for (int i = 0; i < data.npoints; i++)
        dt.push_back(data.data[0][0][0][i]);

    Settings::settings()->setDispersionData(dt);
}

void Analytics::dispersionMplus() {
    Data data = dispersionGenerateData();
    QVector<double> dt = Settings::settings()->dispersionData();
    for (int i = 0; i < data.npoints; i++)
        data.data[0][0][0][i] += dt[i];

    window->regenerate(data);
}

void Analytics::addPulsarCatalog() {
    QString catalog = QFileDialog::getExistingDirectory(this, QString("Pulsar directory"), QSettings().value("openPath").toString());
    if (catalog != "") {
        QSettings().setValue("openPath", MainWindow::nativeDecodeLastPath(catalog));

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

void Analytics::loadFourierData(bool cashOnly) {
    qDebug() << "fourier load called";

    ui->pulsarsTotal->hide();
    ui->progressBar->show();
    ui->currentFile->show();
    ui->currentFile->setText("Releasing previous data");

    QString path = QDir(folder).absolutePath() + "/";
    for (int j = 0; j < fourierData.size(); j++) {
        ui->progressBar->setValue(100 * j / fourierData.size());
        fourierData[j].releaseData();
    }
    fourierData.clear();

    for (int i = 0; i < pulsars->size(); i++) {
        (*pulsars)[i].data.releaseProtected = false;
        (*pulsars)[i].data.releaseData();
    }

    pulsars->clear();



    ui->currentFile->setText("Reading files");

    int blockNumber = ui->fourierBlockNo->value();
    QString cashPath = path + "cash/";
    QString cashFile = cashPath + QString::number(blockNumber);

    if (ui->fourierCashOnly->isChecked() && QFile::exists(cashFile)) {
        QFile cash(cashFile);
        cash.open(QIODevice::ReadOnly);
        QDataStream stream(&cash);
        while (!stream.atEnd()) {
            Pulsar p;
            p.load(stream);
            p.data.releaseProtected = true;
            pulsars->push_back(p);
        }
    } else {
        QString currentPath = path + QString::number(blockNumber) + "/";
        QStringList names = QDir(currentPath).entryList();
        for (int module = 0; module < 6; module++)
            for (int ray = 0; ray < 8; ray++)
                fourierRawNoises[module][ray].clear();

        parseFourierAllowedNames();
        for (int j = 0; j < names.size(); j++)
            if (fourierAllowedNames.contains(names[j])) {
                ui->progressBar->setValue(100 * j / names.size());
                QApplication::processEvents();

                fourierData.push_back(Reader().readBinaryFile(currentPath + names[j]));
                if (fourierData[0].previousLifeName.endsWith(".pnthr")) {
                    Settings::settings()->setFourierStepConstant(0.0124928);
                    Settings::settings()->setFourierSpectreSize(8192);
                    fourierSpectreSize = 8192;
                    longData = true;
                } else {
                    Settings::settings()->setFourierStepConstant(0.0999424);
                    Settings::settings()->setFourierSpectreSize(1024);
                    fourierSpectreSize = 1024;
                    longData = false;
                }

                for (int module = 0; module < 6; module++)
                    for (int ray = 0; ray < 8; ray++) {
                        double noise = 0;
                        Data &data = fourierData.last();
                        for (int channel = 0; channel < data.channels - 1; channel++)
                            for (int point = 0; point < data.npoints; point++)
                                noise += pow(data.data[module][channel][ray][point], 2);

                        data.sigma = -noise;
                        fourierRawNoises[module][ray].push_back(noise);
                    }
            }

        for (int module = 0; module < 6; module++)
            for (int ray = 0; ray < 8; ray++)
                std::sort(fourierRawNoises[module][ray].begin(), fourierRawNoises[module][ray].end());

        ui->currentFile->setText("Running fourier");
        QVector<double> fourierNoises[6][8];

        for (int j = 0; j < fourierData.size(); j++) {
            ui->progressBar->setValue(100 * j / fourierData.size());

            for (int module = 0; module < 6; module++)
                for (int ray = 0; ray < 8; ray++) {
                    Data data;
                    data.npoints = fourierSpectreSize;
                    data.modules = 1;
                    data.rays = 1;
                    data.channels = 1;
                    data.init();
                    data.releaseProtected = true;
                    data.previousLifeName = "file " + fourierData[j].name + " from " + fourierData[j].previousLifeName;
                    data.sigma = fourierData[j].sigma;

                    if (longData) {
                        memcpy(data.data[0][0][0], fourierData[j].data[module][0][ray], sizeof(float) * fourierSpectreSize);
                    } else {
                        QVector<float> dt(fourierSpectreSize, 0);
                        for (int channel = 0; channel < 6; channel++) {
                            Fourier::FFTAnalysis(fourierData[j].data[module][channel][ray], data.data[0][0][0], fourierSpectreSize * 2, fourierSpectreSize);
                            for (int k = 0; k < fourierSpectreSize; k++)
                                dt[k] += data.data[0][0][0][k];
                        }

                        for (int k = 0; k < fourierSpectreSize; k++)
                            data.data[0][0][0][k] = dt[k];
                    }

                    Pulsar pl;
                    pl.module = module + 1;
                    pl.ray = ray + 1;
                    pl.snr = -666;
                    pl.filtered = true;
                    pl.data = data;
                    pl.valid = true;
                    pl.findFourierData(ui->fourierPointsToSkip->value());
                    QTime time(0, 0, 0);
                    pl.nativeTime = time.addSecs(fourierSpectreSize * 2 * blockNumber * Settings::settings()->getFourierStepConstant());
                    pulsars->push_back(pl);

                    if (pl.snr < -660)
                        pl.data.sigma = 1000000000;

                    fourierNoises[module][ray].push_back(pl.noiseLevel);
                }
        }

        for (int module = 0; module < 6; module++)
            for (int ray = 0; ray < 8; ray++)
                std::sort(fourierNoises[module][ray].begin(), fourierNoises[module][ray].end());

        for (int i = 0; i < pulsars->size(); i++) {
            int module = pulsars->at(i).module - 1;
            int ray = pulsars->at(i).ray - 1;
            double avNoise = fourierNoises[module][ray].at(fourierNoises[module][ray].size() / 2);
            if (pulsars->at(i).noiseLevel > avNoise * 1.3)
                (*pulsars)[i].snr = -42;
        }

        std::sort(pulsars->data(), pulsars->data() + pulsars->size());

        applyFourierFilters();

        if (cashOnly || ui->fourierCashResult->isChecked()) {
            QDir().mkpath(cashPath);
            QFile f(cashFile);
            if (f.open(QIODevice::WriteOnly)) {
                QDataStream s(&f);
                for (int i = 0; i < pulsars->size(); i++)
                    if (!pulsars->at(i).filtered)
                        pulsars->at(i).save(s);
            }

            f.close();
        }
    }

    pulsarsEnabled.resize(pulsars->size());

    ui->fourierLoad->setDisabled(true);
    ui->fourierLoad->setText("Data loaded");
    ui->currentFile->hide();

    ui->pulsarsTotal->setText(QString("Loaded %1 files").arg(fourierData.size()));
    ui->pulsarsTotal->show();

    if (!cashOnly)
        apply();
}

void Analytics::actualFourierDataChanged() {
    ui->fourierLoad->setEnabled(true);
    ui->fourierLoad->setText("Load data");

    int t = (ui->fourierBlockNo->value() + 0.5) * fourierSpectreSize * 2 * Settings::settings()->getFourierStepConstant();
    ui->fourierTime->setText(QTime(t / 3600, t / 60 % 60, t % 60).toString("HH:mm:ss"));
}

void Analytics::applyFourierFilters() {
    if (ui->fourierCashOnly->isChecked())
        return;

    QVector<Pulsar>::Iterator end, start = pulsars->begin();
    end = start;
    while (end != pulsars->end() && end->filtered == false)
        end++;

    pulsars->erase(start, end);

    for (int i = 0; i < pulsars->size(); i++)
        (*pulsars)[i].dispersion = 1;

    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 8; j++) {
            fourierSumm[i][j].resize(fourierSpectreSize);
            fourierSumm[i][j].fill(0);
        }

    QVector<bool> good(pulsars->size(), !ui->fourierPeak->isChecked());

    if (ui->fourierPeak->isChecked()) {
        int start = fourierSpectreSize * 2 / ui->fourierPeakTo->value()*Settings::settings()->getFourierStepConstant();
        int end = fourierSpectreSize * 2 / ui->fourierPeakFrom->value()*Settings::settings()->getFourierStepConstant();
        for (int i = 0; i < pulsars->size(); i++) {
            for (int j = start; j < end; j++)
                if ((pulsars->at(i).data.data[0][0][0][j] - pulsars->at(i).fourierAverage) / pulsars->at(i).fourierRealNoise > ui->fourierPeakSNR->value())
                    good[i] = true;
        }
    }

    if (ui->fourierSelectBest->isChecked()) {
        int top = ui->fourierBestNumber->value();
        if (top < 1)
            top = 1;
        if (top > 500)
            top = 500;

        for (int i = 0; i < pulsars->size(); i++) {
            double eqt = fourierRawNoises[pulsars->at(i).module - 1]
                                [pulsars->at(i).ray - 1][top - 1];

            if (-pulsars->at(i).data.sigma <= eqt)
                good[i] = true;
            else
                good[i] = false;
        }
    }

    for (int i = 0; i < pulsars->size(); i++) {
        if (ui->fourierPeak->isChecked())
            (*pulsars)[i].dispersion = 0;

        if (ui->fourierSelectBest->isChecked())
            (*pulsars)[i].dispersion = good[i];

        if ((!ui->fourierGoodLookingSpectresOnly->isChecked() || pulsars->at(i).snr > 0) && good[i]) {
            int module = pulsars->at(i).module - 1;
            int ray = pulsars->at(i).ray - 1;
            for (int k = 0; k < fourierSpectreSize; k++)
                fourierSumm[module][ray][k] += pulsars->at(i).data.data[0][0][0][k];


            if (ui->fourierPeak->isChecked() || ui->fourierSelectBest->isChecked())
                (*pulsars)[i].dispersion = (int)good[i];
        }
    }

    for (int i = 0; i < pulsars->size(); i++)
        if (!good[i] || pulsars->at(i).snr == -666) {
            (*pulsars)[i].dispersion = -7777;
            good[i] = false;
        }

    for (int module = 5; module >= 0; module--)
        for (int ray = 7; ray >= 0; ray--) {
            Data data;
            data.npoints = fourierSpectreSize;
            data.modules = 1;
            data.rays = 1;
            data.channels = 1;
            data.init();
            data.releaseProtected = true;
            memcpy(data.data[0][0][0], fourierSumm[module][ray].constData(), sizeof(float) * fourierSpectreSize);


            Pulsar pl;
            pl.module = module + 1;
            pl.ray = ray + 1;
            pl.snr = -666;
            pl.filtered = false;
            pl.data = data;
            pl.valid = true;
            if (pulsars->size())
                pl.nativeTime = pulsars->last().nativeTime;

            pl.findFourierData(ui->fourierPointsToSkip->value());
            pl.data.sigma = pl.firstPoint;
            pulsars->push_front(pl);

            if (ui->fourierAllPeaks->isChecked())
                while (pl.snr > std::max(ui->SNR->value(), 5.0)) {
                    pl.data.fork();
                    pl.data.releaseProtected = true;
                    pl.snr = -777;
                    pl.findFourierData(pl.firstPoint + (3 + longData * 10));
                    pl.data.sigma = pl.firstPoint;
                    if (pl.snr > 0)
                        pulsars->push_front(pl);
                    else {
                        pl.data.releaseProtected = false;
                        pl.data.releaseData();
                    }
                }
        }

    pulsarsEnabled.resize(pulsars->size());
}

void Analytics::calculateCashes() {
    static bool calculating = false;
    if (calculating) {
        calculating = false;
        ui->fourierCalculateCashes->setText("Stopping...");
        return;
    } else
        calculating = true;

    ui->fourierCalculateCashes->setText("Stop calculating");
    for (int i = 1; i < 425; i++) {
        if (!calculating)
            break;

        ui->fourierBlockNo->setValue(i);
        loadFourierData(true);
    }

    ui->fourierCalculateCashes->setText("Calculate cashes");
}

void Analytics::parseFourierAllowedNames() {
    QString data = ui->fourierAllowedNames->document()->toPlainText();
    data.replace('-', ' ');
    data.replace('\n', ' ');
    fourierAllowedNames.clear();
    QTextStream stream(&data);
    while (!stream.atEnd()) {
        int a, b;
        stream >> a >> b;
        if (a < 0)
            a = 0;
        if (b > 2000)
            b = 2000;

        for (int i = a; i <= b; i++)
            fourierAllowedNames << QString::number(i) + ".pnt";
    }
}

void Analytics::closeEvent(QCloseEvent *) {
    QSettings s;
    s.setValue("AnalyticsGeometry", this->saveGeometry());
}

Analytics::~Analytics()
{
    delete ui;
}
