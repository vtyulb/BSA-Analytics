#include "analytics.h"
#include "ui_analytics.h"

#include <pulsarreader.h>
#include <pulsarworker.h>
#include <pulsarlist.h>
#include <settings.h>
#include <fourier.h>

#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSet>
#include <QTextBrowser>
#include <QStandardPaths>
#include <QDesktopServices>

#include <algorithm>

using std::max;

Analytics::Analytics(QString analyticsPath, bool fourier, QWidget *parent) :
    folder(analyticsPath),
    fourier(fourier),
    oneWindowMode(false),
    QWidget(parent),
    ui(new Ui::Analytics),
    pulsars(new QVector<Pulsar>),
    list(NULL),
    totalFilesLoaded(0),
    noises(new KnownNoise(this))
{
    ui->setupUi(this);
    progressBar = ui->progressBar;

    QObject::connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));
    QObject::connect(ui->addPulsarCatalog, SIGNAL(clicked()), this, SLOT(addPulsarCatalog()));
    QObject::connect(ui->infoButton, SIGNAL(clicked()),this, SLOT(showInfo()));
    QObject::connect(ui->knownPulsarsButton, SIGNAL(clicked()), this, SLOT(knownPulsarsGUI()));
    QObject::connect(ui->knownNoiseButton, SIGNAL(clicked()), noises, SLOT(show()));

    QObject::connect(ui->dispersionPlotButton, SIGNAL(clicked()), this, SLOT(dispersionPlot()));
    QObject::connect(ui->dispersionM, SIGNAL(clicked()), this, SLOT(dispersionMplus()));
    QObject::connect(ui->dispersionRemember, SIGNAL(clicked()), this, SLOT(dispersionRemember()));

    QObject::connect(ui->fourierBlockNo, SIGNAL(valueChanged(int)), this, SLOT(actualFourierDataChanged()));
    QObject::connect(ui->fourierLoad, SIGNAL(clicked()), this, SLOT(loadFourierData()));
    QObject::connect(ui->fourierCalculateCaches, SIGNAL(clicked()), this, SLOT(calculateCaches()));
    QObject::connect(ui->fourierLoadCache, SIGNAL(clicked()), this, SLOT(loadFourierCache()));

    QObject::connect(ui->oneWindow, SIGNAL(clicked()), this, SLOT(oneWindow()));

    maxModule = 6;
    maxRay = 8;
    fourierSpectreSize = 1024;

    fileNames.push_back("all files");

    setAttribute(Qt::WA_DeleteOnClose);

    if (fourier) {
        Settings::settings()->setFourierAnalytics(true);
        ui->groupBox_2->hide();
        ui->groupBox->hide();
        ui->groupBox_4->hide();
        ui->fileNames->hide();
        ui->fourierShortGrayZone->setDisabled(true);

        ui->infoButton->hide();
        ui->addPulsarCatalog->hide();

        ui->doublePeriods->hide();
        ui->duplicatesIterations->hide();
        ui->label_9->hide();

        ui->widget_5->layout()->addWidget(ui->knownPulsarsAndNoises);

        QObject::connect(ui->fourierShortGrayZone, SIGNAL(clicked(bool)), this, SLOT(fourierShortGrayZone()));
        QObject::connect(ui->fourierFullGrayZone, SIGNAL(clicked(bool)), this, SLOT(fourierFullGrayZone()));

        setWindowTitle("Fourier Analytics");
        fourierData.resize(500);
    } else {
        ui->groupBox_5->hide();
        ui->groupBox_6->hide();
    }

    this->restoreGeometry(QSettings().value("AnalyticsGeometry").toByteArray());

    show();
    init();

    resize(minimumWidth(), height());
}

void Analytics::init() {
    QSettings s;
    if (folder == "")
        folder = QFileDialog::getExistingDirectory(this, "Path to *.pulsar files", s.value("openPath").toString());

    s.setValue("openPath", folder);

    window = new MainWindow("", this);
    window->show();

    loadPulsars(folder);
    loadKnownPulsars();
    if (!fourier) {
        apply();
        if (pulsars->size() <= 200)
            preciseDataMode();
    }

    QObject::connect(ui->fileNames, SIGNAL(currentIndexChanged(int)), this, SLOT(apply()));
    progressBar->hide();
    ui->currentStatus->hide();
}

void Analytics::loadKnownPulsars() {
    knownPulsars.clear();
    static bool firstLoad = true;
    QString fileName = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/known-pulsars.txt";
    QFile f(fileName);
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
            if (time.size() < 6) {
                if (firstLoad)
                    qDebug() << "known-pulsars.txt line" << line << "damaged";
                continue;
            }

            pulsar.time = QTime::fromString(time, "hh:mm:ss");
            if (firstLoad)
                qDebug() << "known pulsar" << pulsar.module << pulsar.ray << pulsar.period << pulsar.time << "loaded";

            knownPulsars.push_back(pulsar);
        }
    } else {
        QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        if (!f.open(QIODevice::WriteOnly))
            qDebug() << "can't create file" << fileName;
        else {
            f.write("module\tray\tperiod\ttime\n");
            f.write("#This is commented line\n");
            f.write("6\t7\t1.336\t19:20:18\n");
        }
    }

    firstLoad = false;
}

void Analytics::loadPulsars(QString dir) {
    qDebug() << "scanning directory" << dir;

    if (fourier) {
        loadFourierData();
        return;
    }

    catalogs.push_back(dir);

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
        progressBar->setValue((i + 1) * 100 / list.size());
        qApp->processEvents();
        ui->currentStatus->setText(list[i].fileName());
        Pulsars p = PulsarReader::ReadPulsarFile(list[i].absoluteFilePath(), progressBar);
        Settings::settings()->setFourierHighGround(list[i].fileName().contains("N1"));
        *pulsars += (*p);
        delete p;

        preCalc();
        if (Settings::settings()->lowMemory())
            for (static int j = 0; j < pulsars->size(); j++)
                (*pulsars)[j].squeeze();

        totalFilesLoaded++;
        ui->currentStatus->setText(QString("Loaded %1 pulsar files").arg(totalFilesLoaded));
    }

    pulsarsEnabled.resize(pulsars->size());
}

void Analytics::apply(bool fullFilters) {
    QSize currentSize = this->size();
    if (oneWindowMode)
        progressBar = Settings::settings()->getProgressBar();
    else
        progressBar = ui->progressBar;

    progressBar->setValue(0);
    progressBar->show();

    loadKnownPulsars();
    if (fourier && fullFilters)
        applyFourierFilters();

    pulsarsEnabled.resize(pulsars->size());
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
        QMessageBox::warning(this, "Houston... We've Got a Problem", "There are no such pulsars");
        return;
    }

    if (!fourier)
        std::sort(pl->data(), pl->data() + pl->size());

    if (!list) {
        list = new PulsarList(pl, ui->fourierRemoveBadRawData->isChecked(), this);
        QObject::connect(list, SIGNAL(switchData(Data&)), window, SLOT(regenerate(Data&)));
        QObject::connect(window, SIGNAL(destroyed(QObject*)), list, SLOT(deleteLater()));
        QObject::connect(window, SIGNAL(destroyed(QObject*)), qApp, SLOT(quit()));
        list->show();
    } else
        list->init(pl, ui->fourierRemoveBadRawData->isChecked());

    progressBar->hide();

    if (fourier)
        ui->currentStatus->setText(QString("Loaded %1 files").arg(fourierData.size()));
    else
        ui->currentStatus->setText(QString("Loaded %1 files").arg(totalFilesLoaded));

    ui->currentStatus->show();
    resize(currentSize);
    list->setFocus();
}

void Analytics::applyFileNameFilter() {
    if (fourier)
        return;

    int currentIndex = ui->fileNames->currentIndex();
    if (currentIndex == -1)
        return;

    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pulsarsEnabled[i] &= (pulsars->at(i).data.name == fileNames[currentIndex]);
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
        progressBar->setValue(100 * module / 6);
        update();
        qApp->processEvents();

        for (int ray = 0; ray < 8; ray++)
            for (int k = 0; k < pl[module][ray].size(); k++)
                for (int l = k + 1; l < pl[module][ray].size(); l++) {
                    int i = pl[module][ray][k];
                    int j = pl[module][ray][l];

                    if (abs(pulsars->at(i).nativeTime.secsTo(pulsars->at(j).nativeTime)) < 120 &&
                            globalGoodDoubles(pulsars->at(i).period, pulsars->at(j).period, ui->doublePeriods->isChecked()) &&
                            (pulsars->at(i).data.name != pulsars->at(j).data.name &&
                            !set[i].contains(pulsars->at(j).data.name) &&
                            !set[j].contains(pulsars->at(i).data.name) || fourier)) {
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
    if (!dt.size()) {
        QMessageBox::warning(this, "error", "Memory is not filled now");
        return;
    }

    for (int i = 0; i < data.npoints; i++) {
        double tmp = dt[i];
        dt[i] += data.data[0][0][0][i];
        data.data[0][0][0][i] += tmp;
    }

    Settings::settings()->setDispersionData(dt);

    window->regenerate(data);
}

void Analytics::addPulsarCatalog() {
    QString catalog = QFileDialog::getExistingDirectory(this, QString("Pulsar directory"), QSettings().value("openPath").toString());
    if (catalog != "") {
        QSettings().setValue("openPath", MainWindow::nativeDecodeLastPath(catalog));

        progressBar->show();
        ui->currentStatus->show();
        loadPulsars(catalog);
        ui->currentStatus->hide();
        progressBar->hide();
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
    QString fileName = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/known-pulsars.txt";
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void Analytics::applyKnownNoiseFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        pulsarsEnabled[i] &= !noises->contains(pulsars->at(i).period);
}

void Analytics::loadFourierCache() {
    loadFourierData(false, true);
}

void Analytics::loadFourierData(bool cacheOnly, bool loadCache) {
    qDebug() << "fourier load called";

    progressBar->show();
    ui->currentStatus->show();
    ui->currentStatus->setText("Releasing previous data");

    QString path = QDir(folder).absolutePath() + "/";
    for (int j = 0; j < fourierData.size(); j++) {
        progressBar->setValue(100 * j / fourierData.size());
        fourierData[j].releaseData();
    }
    fourierData.clear();

    for (int i = 0; i < pulsars->size(); i++) {
        if ((*pulsars)[i].fourierDuplicate)
            continue;

        (*pulsars)[i].data.releaseProtected = false;
        (*pulsars)[i].data.releaseData();
    }

    pulsars->clear();

    ui->currentStatus->setText("Reading files");

    int blockNumber = ui->fourierBlockNo->value();
    QString cachePath = path + "cach/";
    QString cacheFile = cachePath + QString::number(blockNumber);

    if (loadCache && QFile::exists(cacheFile)) {
        QFile cache(cacheFile);
        cache.open(QIODevice::ReadOnly);
        QDataStream stream(&cache);
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

        for (int j = 0; j < names.size(); j++)
            {
                progressBar->setValue(100 * j / names.size());
                QApplication::processEvents();

                fourierData.push_back(Reader().readBinaryFile(currentPath + names[j]));
                Settings::settings()->setFourierHighGround(fourierData.first().previousLifeName.contains("N1"));
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

                Data &data = fourierData.last();
                if (data.modules != 6 || data.rays != 8) {
                    qDebug() << "bad data detected" << data.modules << data.rays << data.name << data.previousLifeName;
                    fourierData.removeLast();
                    continue;
                }

                for (int module = 0; module < 6; module++)
                    for (int ray = 0; ray < 8; ray++) {
                        double noise = 0;                        
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

        ui->currentStatus->setText("Running fourier");
        QVector<double> fourierNoises[6][8];

        for (int j = 0; j < fourierData.size(); j++) {
            progressBar->setValue(100 * j / fourierData.size());

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
                    pl.data.sigma = pl.firstPoint;
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

        if (cacheOnly || ui->fourierGenerateCache->isChecked()) {
            QDir().mkpath(cachePath);
            QFile f(cacheFile);
            if (f.open(QIODevice::WriteOnly)) {
                QDataStream s(&f);
                for (int i = 0; i < pulsars->size(); i++)
                    if (!pulsars->at(i).filtered)
                        pulsars->at(i).save(s);
            } else
                qDebug() << "can't create cache file: " << cacheFile;

            f.close();
        }
    }

    pulsarsEnabled.resize(pulsars->size());

    ui->fourierLoad->setDisabled(true);
    ui->fourierLoad->setText("Data loaded");
    ui->currentStatus->setText(QString("Loaded %1 files").arg(fourierData.size()));

    if (!cacheOnly) {
        if (!ui->fourierFullGrayZone->isEnabled())
            fourierFullGrayZone();
        else
            apply(false);
    }
}

void Analytics::actualFourierDataChanged() {
    ui->fourierLoad->setEnabled(true);
    ui->fourierLoad->setText("Load data");

    int t = (ui->fourierBlockNo->value() + 0.5) * fourierSpectreSize * 2 * Settings::settings()->getFourierStepConstant();
    ui->fourierTime->setText(QTime(t / 3600, t / 60 % 60, t % 60).toString("HH:mm:ss"));
}

void Analytics::applyFourierFilters() {
    if (!fourier)
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
        int start = fourierSpectreSize * 2 / ui->fourierPeakAt->value()*Settings::settings()->getFourierStepConstant() - 2;
        int end = fourierSpectreSize * 2 / ui->fourierPeakAt->value()*Settings::settings()->getFourierStepConstant() + 3;
        for (int i = 0; i < pulsars->size(); i++) {
            for (int j = start; j < end; j++)
                if ((pulsars->at(i).data.data[0][0][0][j] - pulsars->at(i).fourierAverage) / pulsars->at(i).fourierRealNoise > ui->fourierPeakSNR->value())
                    good[i] = true;
        }
    }

    if (ui->fourierAllowedDatesCheckbox->isChecked()) {
        parseFourierAllowedDates();
        for (int i = 0; i < pulsars->size(); i++) {
            bool gd = false;
            QDate date = pulsars->at(i).data.dateFromPreviousLifeName();
            for (int j = 0; j < fourierAllowedDates.size(); j += 2)
                gd = gd || (fourierAllowedDates[j] < date && date < fourierAllowedDates[j + 1]);

            good[i] = good[i] && gd;
        }
    }

    if (ui->fourierOnlyNightData->isChecked()) {
        for (int i = 0; i < pulsars->size(); i++) {
            int hour = pulsars->at(i).data.hourFromPreviousLifeName();
            good[i] = good[i] && (1 <= hour && hour <= 6);
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

        if ((!ui->fourierGoodLookingSpectresOnly->isChecked() || pulsars->at(i).snr > 0) && good[i] && !pulsars->at(i).fourierDuplicate) {
            int module = pulsars->at(i).module - 1;
            int ray = pulsars->at(i).ray - 1;
            for (int k = 0; k < fourierSpectreSize; k++)
                fourierSumm[module][ray][k] += pulsars->at(i).data.data[0][0][0][k];


            if (ui->fourierPeak->isChecked() || ui->fourierSelectBest->isChecked())
                (*pulsars)[i].dispersion = (int)good[i];
        }
    }

    for (int i = 0; i < pulsars->size(); i++)
        if (!good[i] || pulsars->at(i).snr == -666 || pulsars->at(i).snr == -42) {
            (*pulsars)[i].dispersion = -7777;
            good[i] = false;
        }

    ui->currentStatus->setText("Generating white zone");
    QVector<Pulsar> *whiteZone = new QVector<Pulsar>;
    for (int module = 5; module >= 0; module--)
        for (int ray = 7; ray >= 0; ray--) {
            progressBar->setValue(100 * (7 - ray + (5 - module) * 8) / 48);
            Data data;
            data.npoints = fourierSpectreSize;
            data.modules = 1;
            data.rays = 1;
            data.channels = 1;
            data.previousLifeName = "whitezone";
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
            whiteZone->push_front(pl);

            if (ui->fourierAllPeaks->isChecked())
                while (pl.snr > std::max(ui->SNR->value(), 5.0)) {
                    pl.data.fork();
                    pl.data.releaseProtected = true;
                    pl.snr = -777;
                    pl.findFourierData(pl.firstPoint + (3 + longData * 10));
                    pl.data.sigma = pl.firstPoint;
                    if (pl.snr > 0)
                        whiteZone->push_front(pl);
                    else {
                        pl.data.releaseProtected = false;
                        pl.data.releaseData();
                    }
                }
        }

    for (int i = 0; i < pulsars->size(); i++)
        whiteZone->push_back(pulsars->at(i));

    delete pulsars;
    pulsars = whiteZone;

    progressBar->hide();
    qApp->processEvents();
    pulsarsEnabled.resize(pulsars->size());
}

void Analytics::calculateCaches() {
    static bool calculating = false;
    if (calculating) {
        calculating = false;
        ui->fourierCalculateCaches->setText("Stopping...");
        return;
    } else
        calculating = true;

    ui->fourierCalculateCaches->setText("Stop calculating");
    for (int i = 1; i < 425; i++) {
        if (!calculating)
            break;

        ui->fourierBlockNo->setValue(i);
        loadFourierData(true);
    }

    ui->fourierCalculateCaches->setText("Calculate caches");
}

void Analytics::parseFourierAllowedDates() {
    QString data = ui->fourierAllowedDates->text();
    data.replace('-', ' ');
    data.replace('\n', ' ');
    data.replace(',', ' ');
    data.replace(';', ' ');
    data.replace("  ", " ");

    QStringList dates = data.split(' ');

    fourierAllowedDates.clear();

    for (int i = 0; i < dates.size(); i++)
        if (dates[i].size() > 6)
            fourierAllowedDates.push_back(QDate::fromString(dates[i], "dd.MM.yyyy"));

    fourierAllowedDates.resize(fourierAllowedDates.size() / 2 * 2);

    qDebug() << "parsed dates";
    for (int i = 0; i < fourierAllowedDates.size(); i += 2)
        qDebug() << fourierAllowedDates[i].toString() << "-" << fourierAllowedDates[i + 1].toString();

    qDebug() << "============";
}

void Analytics::fourierShortGrayZone() {
    ui->fourierShortGrayZone->setDisabled(true);
    ui->fourierFullGrayZone->setEnabled(true);

    while (pulsars->last().fourierDuplicate)
        pulsars->removeLast();

    ui->groupBox->hide();
    apply();
}

void Analytics::fourierFullGrayZone() {
    ui->fourierFullGrayZone->setDisabled(true);

    ui->currentStatus->setText("Generating yellow zone");
    ui->currentStatus->show();
    progressBar->show();
    int size = pulsars->size();
    for (int i = 0; i < size; i++)
        if (pulsars->at(i).filtered && pulsars->at(i).snr > 2) {
            if (i % 100 == 0) {
                progressBar->setValue(100 * i / size);
                qApp->processEvents();
            }

            Pulsar pl = pulsars->at(i);

            pl.findFourierData(ui->fourierPointsToSkip->value());
            pl.data.sigma = pl.firstPoint;
            pl.fourierDuplicate = true;
            pulsars->push_back(pl);

            while (pl.snr > 5.0) {
                pl.data.releaseProtected = true;
                pl.snr = -777;
                pl.findFourierData(pl.firstPoint + (3 + longData * 10));
                pl.data.sigma = pl.firstPoint;
                if (pl.snr > 0)
                    pulsars->push_back(pl);
            }
        }

    ui->currentStatus->hide();
    progressBar->hide();
    ui->fourierShortGrayZone->setEnabled(true);

    ui->groupBox->show();
    apply();
}

void Analytics::preciseDataMode() {
    ui->groupBox->hide();
    ui->differentNoise->setEnabled(false);
    ui->module->setEnabled(false);
    ui->moduleCheckBox->setEnabled(false);
    ui->ray->setEnabled(false);
    ui->rayCheckBox->setEnabled(false);
    ui->period->setEnabled(false);
    ui->periodCheckBox->setEnabled(false);
    ui->periodRangeLeft->setEnabled(false);
    ui->periodRangeRight->setEnabled(false);
    ui->periodRangeCheckBox->setEnabled(false);
    ui->time->setEnabled(false);
    ui->timeCheckBox->setEnabled(false);
    ui->multiplePicks->setEnabled(false);
    ui->strangeData->setEnabled(false);
    ui->differentNoise->setEnabled(false);
}

void Analytics::oneWindow() {
    window->setParent(NULL);
    list->setParent(NULL);
    setParent(NULL);
    progressBar = ui->progressBar;
    if (oneWindowMode) {
        ui->oneWindow->setText("One window");
        oneWindowMode = false;
        window->addWidgetToMainLayout(NULL, NULL);
        list->show();
        show();
        return;
    } else
        ui->oneWindow->setText("Gimp mode");

    oneWindowMode = true;
    progressBar = Settings::settings()->getProgressBar();

    setContentsMargins(0, 0, 0, 0);
    list->setContentsMargins(0, 0, 0, 0);

    window->showMaximized();
    QTimer::singleShot(500, window, SLOT(update()));
    window->addWidgetToMainLayout(this, list);
    window->show();
}

Analytics::~Analytics() {
    qDebug() << "analytics destroyed";
    delete ui;
}
