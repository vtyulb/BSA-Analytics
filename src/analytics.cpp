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
#include <QEventLoop>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSet>
#include <QTextBrowser>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUuid>

#include <algorithm>

using std::max;

Analytics::Analytics(QString analyticsPath, bool fourier, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Analytics),
    list(NULL),
    knownNoises(new KnownNoise(this)),
    folder(QDir().absoluteFilePath(analyticsPath)),
    oneWindowMode(false),
    pulsars(new QVector<Pulsar>),
    fourier(fourier),
    transient(false),
    longData(false),
    totalFilesLoaded(0),
    cacheLoaded(false)
{
    ui->setupUi(this);
    progressBar = ui->progressBar;

    QObject::connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(apply()));
    QObject::connect(ui->addPulsarCatalog, SIGNAL(clicked()), this, SLOT(addPulsarCatalog()));
    QObject::connect(ui->infoButton, SIGNAL(clicked()),this, SLOT(showInfo()));
    QObject::connect(ui->knownPulsarsButton, SIGNAL(clicked()), this, SLOT(knownPulsarsGUI()));

    QObject::connect(ui->dispersionPlotButton, SIGNAL(clicked()), this, SLOT(dispersionPlot()));
    QObject::connect(ui->dispersionM, SIGNAL(clicked()), this, SLOT(dispersionMplus()));
    QObject::connect(ui->dispersionRemember, SIGNAL(clicked()), this, SLOT(dispersionRemember()));

    QObject::connect(ui->profileRemember, SIGNAL(clicked()), this, SLOT(profileRemember()));
    QObject::connect(ui->profileM, SIGNAL(clicked()), this, SLOT(profileMplus()));

    QObject::connect(ui->fourierBlockNo, SIGNAL(valueChanged(int)), this, SLOT(actualFourierDataChanged()));
    QObject::connect(ui->fourierLoad, SIGNAL(clicked()), this, SLOT(loadFourierData()));
    QObject::connect(ui->fourierCalculateCaches, SIGNAL(clicked()), this, SLOT(calculateCaches()));
    QObject::connect(ui->fourierLoadCache, SIGNAL(clicked()), this, SLOT(loadFourierCache()));

    QObject::connect(ui->fourierShowSpectresNoise, SIGNAL(clicked()), this, SLOT(fourierShowSpectresNoise()));
    QObject::connect(ui->fourierShowNoises, SIGNAL(clicked()), this, SLOT(fourierShowNoises()));
    QObject::connect(ui->fourierSelectBestAuto, SIGNAL(clicked(bool)), this, SLOT(fourierSelectBestAuto()));
    QObject::connect(ui->fourierSelectBest, SIGNAL(toggled(bool)), this, SLOT(fourierSelectBestEnabled(bool)));

    QObject::connect(ui->oneWindow, SIGNAL(clicked()), this, SLOT(oneWindow()));

    maxModule = 6;
    maxRay = 8;
    fourierSpectreSize = 1024;

    fileNames.push_back("all files");

    setAttribute(Qt::WA_DeleteOnClose);

    if (QDir(folder).entryList().contains("noises.pnt") || QDir(folder).entryList().contains("noises.pnthr"))
        fourier = true, this->fourier = true;

    if (QDir(folder).entryList().contains("transients"))
        fourier = true, this->fourier = true, transient = true;

    if (fourier) {
        Settings::settings()->setFourierAnalytics(true);
        ui->groupBox->hide();
        ui->groupBox_2->hide();
        ui->groupBox_4->hide();
        ui->groupBox_7->hide();
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
        QObject::connect(ui->knownNoiseButton, SIGNAL(clicked()), this, SLOT(knownPulsarsGUI()));

        setWindowTitle("Fourier Analytics");
        fourierData.resize(500);
    } else {
        ui->groupBox_5->hide();
        ui->groupBox_6->hide();
        QObject::connect(ui->knownNoiseButton, SIGNAL(clicked()), knownNoises, SLOT(show()));
    }

    if (transient) {
        ui->fourierCalculateCaches->hide();
        ui->fourierLoadCache->hide();
        ui->groupBox_6->hide();
        ui->oneWindow->hide();

        QPushButton *saveImage = new QPushButton(this);
        ui->widget_10->layout()->addWidget(saveImage);
        saveImage->setText("Save");
        saveImage->show();
        QObject::connect(saveImage, SIGNAL(clicked()), this, SLOT(transientSaveImage()));

        QPushButton *saveForPublication = new QPushButton(this);
        ui->widget_10->layout()->addWidget(saveForPublication);
        saveForPublication->setText("Save for publication");
        saveForPublication->show();
        QObject::connect(saveForPublication, SIGNAL(clicked()), this, SLOT(transientSaveImageForPublication()));

        ui->fourierNormalizeData->setText("Build whitezone");
        ui->fourierNormalizeData->setChecked(true);
        QObject::connect(ui->fourierNormalizeData, SIGNAL(toggled(bool)), this, SLOT(enableTransientWhitezone(bool)));

        Settings::settings()->setTransientAnalytics(true);
    }

    this->restoreGeometry(QSettings().value("AnalyticsGeometry").toByteArray());

    compressLayout();
    show();
    init();

    resize(minimumWidth(), height());
    if (!QSettings().value("GimpMode", false).toBool() || transient)
        QTimer::singleShot(100, this, SLOT(oneWindow()));

    QDir::setCurrent(qApp->applicationDirPath());
}

void Analytics::init() {
    QSettings s;
    s.setValue("openPath", folder);

    window = new MainWindow("", this);
    window->show();

    loadPulsars(folder);
    loadKnownPulsars();
    if (!fourier) {
        if (pulsars->size() <= 200)
            preciseDataMode();

        apply();
    }

    QObject::connect(ui->fileNames, SIGNAL(currentIndexChanged(int)), this, SLOT(apply()));
    progressBar->hide();
}

void Analytics::loadKnownPulsars() {
    knownPulsars.clear();
    QString fileName = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/known-pulsars.txt";
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QString lastComment;
        int lineNumber = 0;
        while (!f.atEnd()) {
            lineNumber++;
            QByteArray line = f.readLine();
            line.replace("\t", " ");
            line.replace("\n", " ");
            if (line[0] == '#') {
                line.replace("#", "");
                lastComment = QString::fromLocal8Bit(line);
                continue;
            }
            QTextStream stream(&line, QIODevice::ReadOnly);
            KnownPulsar pulsar;
            QString time, module, ray, period;
            stream >> module >> ray >> period >> time;
            if (module == "*")
                pulsar.module = -1;
            else
                pulsar.module = module.toInt();

            if (ray == "*")
                pulsar.ray = -1;
            else
                pulsar.ray = ray.toInt();

            if (period == "*")
                pulsar.period = -1;
            else
                pulsar.period = period.toDouble();

            if (time.size() < 4) {
                if (line.replace(" ", "") != "")
                    QMessageBox::warning(NULL, "Warning", "Error loading file " + fileName + "\n"
                                                          "Line " + QString::number(lineNumber) + " is corrupted\n"
                                                          "Line content: \"" + line + "\"");
                continue;
            }

            pulsar.comment = stream.readAll();
            pulsar.comment = lastComment + "\n" + pulsar.comment;

            time = time.toUpper();
            if (time.contains("LONG"))
                pulsar.longNoise = true;

            if (time.contains("SHORT"))
                pulsar.shortNoise = true;

            if (!pulsar.shortNoise && !pulsar.longNoise) {
                if (time.size() < 6)
                    pulsar.time = QTime::fromString(time, "hh:mm");
                else
                    pulsar.time = QTime::fromString(time, "hh:mm:ss");
            }

            knownPulsars.push_back(pulsar);
        }
    } else {
        QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        if (!f.open(QIODevice::WriteOnly))
            qDebug() << "can't create file" << fileName;
        else {
            QFile tmp(":/other/known-pulsars.txt");
            tmp.open(QIODevice::ReadOnly);
            f.write(tmp.readAll());
            f.close();
        }
    }
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
    ui->currentStatus->setText(QString("Loaded %1 pulsar files").arg(totalFilesLoaded));
}

void Analytics::apply(bool fullFilters) {
    if (list)
        list->setDisabled(true);

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
    pulsarsEnabled.fill(true);

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

    if (ui->knownPulsars->isChecked())
        applyKnownPulsarsFilter();

    if (ui->knownNoise->isChecked()) {
        if (fourier)
            applyFourierKnownNoiseFilter();
        else
            applyKnownNoiseFilter();
    }

    if (ui->differentNoise->isChecked())
        applyDifferentNoise();

    if (ui->strangeData->isChecked())
        applyStrangeDataFilter();

    if (ui->fileNames->currentIndex() != 0)
        applyFileNameFilter();

    if (ui->SNRCheckBox->isChecked())
        applySNRFilter();

    if (ui->differentMaximums->isChecked())
        applyDifferentMaximumsFilter();

    if (ui->duplicates->value() > 1)
        for (int i = 0; i < ui->duplicatesIterations->value(); i++)
            applyDuplicatesFilter();

    Pulsars pl = new QVector<Pulsar>;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            pl->push_back(pulsars->at(i));

    if (!pl->size()) {
//        QMessageBox::warning(this, "Houston... We've Got a Problem", "There are no such pulsars");
        Pulsar p;
        p.data.npoints = 1;
        p.data.modules = 1;
        p.data.channels = 1;
        p.data.rays = 1;
        p.data.init();
        pl->push_back(p);
    }

    if (!fourier)
        std::sort(pl->data(), pl->data() + pl->size());

    if (!list) {
        list = new PulsarList(this);
        list->restoreGeometry(QSettings().value("pulsar-list-geometry").toByteArray());
        QObject::connect(list, SIGNAL(switchData(Data&)), window, SLOT(regenerate(Data&)));
        QObject::connect(list, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        QObject::connect(window, SIGNAL(destroyed(QObject*)), list, SLOT(deleteLater()));
        QObject::connect(window, SIGNAL(destroyed(QObject*)), qApp, SLOT(quit()));
        list->show();
    }

    ui->currentStatus->setText("Generating GUI list");
    progressBar->show();
    qApp->processEvents();
    list->init(pl, ui->fourierRemoveBadRawData->isChecked());

    progressBar->hide();

    if (fourier) {
        QSet<QString> filesUsed;
        for (int i = 0; i < pl->size(); i++)
            if (!pl->at(i).fourierDuplicate && pl->at(i).showInTable)
                filesUsed.insert(pl->at(i).data.previousLifeName);

        filesUsed.remove("whitezone");
        ui->currentStatus->setText(QString("Used %1 of %2 files").arg(filesUsed.size()).arg(fourierData.size()));
    } else
        ui->currentStatus->setText(QString("Loaded %1 files").arg(totalFilesLoaded));

    resize(currentSize);
    list->setEnabled(true);
    list->setFocus();

    window->update();
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
                if (!knownPulsars[j].shortNoise && !knownPulsars[j].longNoise && knownPulsars[j] == pulsars->at(i)) {
                    (*pulsars)[i].isKnownPulsar = true;
                    (*pulsars)[i].knownPulsarComment = knownPulsars[j].comment;
                    break;
                }
}

void Analytics::applyFourierKnownNoiseFilter() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i])
            for (int j = 0; j < knownPulsars.size(); j++)
                if ((knownPulsars[j].shortNoise && !longData) ||
                    (knownPulsars[j].longNoise && longData))
                    if (knownPulsars[j] == pulsars->at(i)) {
                        (*pulsars)[i].showInTable = false;
                        (*pulsars)[i].knownPulsarComment = knownPulsars[j].comment;
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

    ui->currentStatus->setText("Duplicates filter");
    progressBar->show();
    for (int module = 0; module < 6; module++)
        for (int ray = 0; ray < 8; ray++) {
            progressBar->setValue(100 * (module * 8 + ray) / 48);
            qApp->processEvents();

            for (int k = 0; k < pl[module][ray].size(); k++)
                for (int l = k + 1; l < pl[module][ray].size(); l++) {
                    int i = pl[module][ray][k];
                    int j = pl[module][ray][l];

                    if ((!fourier && abs(pulsars->at(i).nativeTime.secsTo(pulsars->at(j).nativeTime)) < 120 &&
                            globalGoodDoubles(pulsars->at(i).period, pulsars->at(j).period, ui->doublePeriods->isChecked()) &&
                            (pulsars->at(i).data.name != pulsars->at(j).data.name &&
                            !set[i].contains(pulsars->at(j).data.name) &&
                            !set[j].contains(pulsars->at(i).data.name))) ||
                            (fourier && pulsars->at(i).period == pulsars->at(j).period)) {
                        set[i].insert(pulsars->at(j).data.name);
                        set[j].insert(pulsars->at(i).data.name);
                        (*pulsars)[i].firstPoint++;
                        (*pulsars)[j].firstPoint++;
                    }
                }
        }
    progressBar->hide();

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

    QObject::disconnect(ui->fileNames, SIGNAL(currentIndexChanged(int)), this, SLOT(apply()));
    ui->fileNames->clear();
    for (int i = 0; i < fileNames.size(); i++)
        ui->fileNames->addItem(fileNames[i]);
    QObject::connect(ui->fileNames, SIGNAL(currentIndexChanged(int)), this, SLOT(apply()));
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
    Data data = Settings::settings()->getLastData();
    QVector<double> dt;
    for (int i = 0; i < data.npoints; i++)
        dt.push_back(data.data[0][0][0][i]);

    Settings::settings()->setDispersionData(dt);
}

void Analytics::dispersionMplus() {
    Data data = dispersionGenerateData();
    QVector<double> dt = Settings::settings()->dispersionData();
    if (!dt.size()) {
        QMessageBox::warning(this, "Error", "Memory is not filled");
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

void Analytics::profileRemember() {
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsarsEnabled[i]) {
            const Data data = pulsars->at(i).data;
            QVector<double> dt;
            for (int i = 0; i < data.npoints; i++)
                dt.push_back(data.data[0][0][0][i]);

            Settings::settings()->setProfileData(dt, pulsars->at(i).dispersion);
        }
}

void Analytics::profileMplus() {
    for (int current = 0; current < pulsars->size(); current++)
        if (pulsarsEnabled[current]) {
            QVector<double> dt = Settings::settings()->profileData(pulsars->at(current).dispersion);
            if (!dt.size()) {
                qDebug() << "no profile at current dispersion" << pulsars->at(current).dispersion << "found";
                continue;
            }

            const Data &data = pulsars->at(current).data;
            for (int i = 0; i < std::min(data.npoints, dt.size()); i++)
                data.data[0][0][0][i] += dt[i];
        }

    profileRemember();

    Data last = Settings::settings()->getLastData();
    window->regenerate(last);
}

void Analytics::addPulsarCatalog() {
    QString catalog = QFileDialog::getExistingDirectory(this, QString("Pulsar directory"), QSettings().value("openPath").toString());
    if (catalog != "") {
        QSettings().setValue("openPath", MainWindow::nativeDecodeLastPath(catalog));
        if (list)
            list->setEnabled(false);

        progressBar->show();
        loadPulsars(catalog);
        progressBar->hide();
    }

    apply();
    list->setEnabled(true);
    list->setFocus();
    window->update();
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
        pulsarsEnabled[i] &= !knownNoises->contains(pulsars->at(i).period);
}

void Analytics::loadFourierCache() {
    loadFourierData(false, true);
}

void Analytics::loadFourierData(bool cacheOnly, bool loadCache) {
    qDebug() << "fourier load called";
    if (list)
        list->setDisabled(true);

    ui->fourierLoad->setDisabled(true);
    ui->fourierLoadCache->setDisabled(true);
    ui->fourierLoad->setText("Loading data");
    ui->fourierLoadCache->setText("Loading data");
    ui->fourierShowNoises->setText("Noises");

    progressBar->show();
    ui->currentStatus->setText("Releasing previous data");

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
    QString path = QDir(folder).absolutePath() + "/";
    QString blockStr = QString::number(blockNumber);
    if (!QDir(path + blockStr).exists())
        blockStr = QString::asprintf("%03d", blockNumber);
    QString cachePath = path + "cache/";
    QString cacheFile = cachePath + blockStr;

    if (loadCache && QFile::exists(cacheFile)) {
        QFile cache(cacheFile);
        cache.open(QIODevice::ReadOnly);
        cacheLoaded = true;
        QDataStream stream(&cache);
        while (!stream.atEnd()) {
            Pulsar p;
            p.load(stream);
            p.data.releaseProtected = true;
            pulsars->push_back(p);
        }
    } else {
        QString currentPath = path + blockStr + "/";
        QStringList names = QDir(currentPath).entryList(QDir::Files);
        for (int module = 0; module < 6; module++)
            for (int ray = 0; ray < 8; ray++)
                fourierRawNoises[module][ray].clear();

        headers.clear();
        for (int j = 0; j < names.size(); j++)
            {
                progressBar->setValue(100 * j / names.size());
                qApp->processEvents();

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
                if ((data.modules != 6 || data.rays != 8) && !transient) {
                    qDebug() << "bad data detected" << data.modules << data.rays << data.name << data.previousLifeName;
                    fourierData.removeLast();
                    continue;
                }

                headers.push_back(Settings::settings()->getLastHeader());

                if (longData) {
                    /*fourierLoadNoises();
                    QStringList names = Settings::settings()->getLastHeader()["stairs_names"].split(",");
                    for (int k = 0; k < names.size(); k++)
                        if (names[k] == data.previousLifeName)
                            for (int module = 0; module < data.modules; module++)
                                for (int ray = 0; ray < data.rays; ray++)
                                    fourierRawNoises[module][ray].push_back(noises.data[module][32][ray][k]);*/
                }

                for (int module = 0; module < data.modules; module++)
                    for (int ray = 0; ray < data.rays; ray++) {
                        double noise = 0;
                        for (int channel = 0; channel < data.channels - 1; channel++) {
                            double chNoise = 0;
                            for (int point = 0; point < data.npoints; point++)
                                chNoise += double(data.data[module][channel][ray][point])*data.data[module][channel][ray][point];

                            chNoise /= data.npoints;
                            chNoise = pow(chNoise, 0.5);

                            if (data.channels > 1 && ui->fourierNormalizeData->isChecked())
                                for (int point = 0; point < data.npoints; point++)
                                    data.data[module][channel][ray][point] /= chNoise;

                            noise += chNoise;
                        }

                        fourierRawNoises[module][ray].push_back(noise);
                    }
            }

        ui->currentStatus->setText("Running fourier");
        for (int j = 0; j < fourierData.size(); j++) {
            progressBar->setValue(100 * j / fourierData.size());
            qApp->processEvents();

            for (int module = 0; module < fourierData[j].modules; module++)
                for (int ray = 0; ray < fourierData[j].rays; ray++) {
                    Data data;
                    data.npoints = fourierSpectreSize * (!transient) + transient * fourierData[j].npoints;
                    data.modules = 1;
                    data.rays = 1;
                    data.channels = (fourierData[j].channels) * (transient) + !transient;
                    data.fbands = fourierData[j].fbands;
                    data.time = fourierData[j].time;
                    data.init();
                    data.releaseProtected = true;
                    data.previousLifeName = "file " + fourierData[j].name + " from " + fourierData[j].previousLifeName;

                    if (longData) {
                        for (int k = 0; k < fourierData[j].channels; k++)
                            memcpy(data.data[0][k][0], fourierData[j].data[module][k][ray], sizeof(float) * fourierData[j].npoints);
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
                    pl.sigma = fourierRawNoises[module][ray][j];
                    if (!transient) {
                        pl.findFourierData(ui->fourierPointsToSkip->value());
                        pl.data.sigma = pl.firstPoint;
                    } else {
                        pl.module = headers[j]["module"].toInt();
                        pl.ray = headers[j]["ray"].toInt();
                        pl.dispersion = headers[j]["dispersion"].toInt();
                        pl.data.sigma = -headers[j]["point"].toInt();
                    }

                    pl.nativeTime = QTime(0, 0).addSecs(int(fourierSpectreSize * 2 * (blockNumber - 0.5) * Settings::settings()->getFourierStepConstant()));
                    pulsars->push_back(pl);

                    if (pl.snr < -660)
                        pl.data.sigma = 1000000000;

                    fourierNoises[module][ray].push_back(pl.noiseLevel);
                }
        }

        if (!transient) {
            for (int module = 0; module < 6; module++)
                for (int ray = 0; ray < 8; ray++)
                    std::sort(fourierRawNoises[module][ray].begin(), fourierRawNoises[module][ray].end());

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
        }

        std::sort(pulsars->begin(), pulsars->end());

        applyFourierFilters();

        if (cacheOnly) {
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

    if (transient && transientWhitezoneEnabled)
        buildTransientWhitezone(pulsars);

    pulsarsEnabled.resize(pulsars->size());

    ui->fourierLoad->setText("Data loaded");
    ui->fourierLoadCache->setText("Data loaded");
    ui->currentStatus->setText(QString("Loaded %1 files").arg(fourierData.size()));

    if (!cacheOnly) {
        if (!ui->fourierFullGrayZone->isEnabled())
            fourierFullGrayZone();
        else
            apply(false);
    }

    list->setEnabled(true);
}

void Analytics::actualFourierDataChanged() {
    ui->fourierLoad->setEnabled(true);
    ui->fourierLoadCache->setEnabled(true);
    ui->fourierLoad->setText("Load data");
    ui->fourierLoadCache->setText("Load cache");

    int secs = (ui->fourierBlockNo->value() - 0.5) * fourierSpectreSize * 2 * Settings::settings()->getFourierStepConstant();
    ui->fourierTime->setText(QTime(0, 0).addSecs(secs).toString("HH:mm:ss"));
}

void Analytics::applyFourierFilters() {
    if (!fourier || (fourierData.size() == 0 && cacheLoaded) || transient)
        return;

    QVector<Pulsar>::Iterator end, start = pulsars->begin();
    end = start;
    while (end != pulsars->end() && end->filtered == false)
        end++;

    for (QVector<Pulsar>::Iterator i = start; i != end; i++) {
        (*i).data.releaseProtected = false;
        (*i).data.releaseData();
    }
    pulsars->erase(start, end);

    for (int i = 0; i < pulsars->size(); i++)
        (*pulsars)[i].dispersion = 1;

    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 8; j++) {
            fourierSumm[i][j].resize(fourierSpectreSize);
            fourierSumm[i][j].fill(0);
        }

    fourierGood.resize(pulsars->size());
    fourierGood.fill(true);
    if (ui->fourierPeak->isChecked()) {
        int start = fourierSpectreSize * 2 / ui->fourierPeakAt->value()*Settings::settings()->getFourierStepConstant() - 2;
        int end = fourierSpectreSize * 2 / ui->fourierPeakAt->value()*Settings::settings()->getFourierStepConstant() + 3;
        for (int i = 0; i < pulsars->size(); i++) {
            bool good = false;
            for (int j = start; j < end; j++)
                if ((pulsars->at(i).data.data[0][0][0][j] - pulsars->at(i).fourierAverage) / pulsars->at(i).fourierRealNoise > ui->fourierPeakSNR->value())
                    good = true;

            fourierGood[i] = good;
        }
    }

    if (ui->fourierAllowedDatesCheckbox->isChecked()) {
        parseFourierAllowedDates();
        for (int i = 0; i < pulsars->size(); i++) {
            bool gd = false;
            QDate date = pulsars->at(i).data.dateFromPreviousLifeName();
            for (int j = 0; j < fourierAllowedDates.size(); j += 2)
                gd = gd || (fourierAllowedDates[j] < date && date < fourierAllowedDates[j + 1]);

            fourierGood[i] = fourierGood[i] && gd;
        }
    }

    if (ui->fourierOnlyNightData->isChecked()) {
        for (int i = 0; i < pulsars->size(); i++) {
            int hour = pulsars->at(i).data.hourFromPreviousLifeName();
            fourierGood[i] = fourierGood[i] && (1 <= hour && hour <= 6);
        }
    }

    if (ui->fourierSelectBest->isChecked()) {
        int top = ui->fourierBestNumber->value();
        for (int i = 0; i < pulsars->size(); i++) {
            int module = pulsars->at(i).module - 1;
            int ray = pulsars->at(i).ray - 1;
            double eqt = fourierRawNoises[module][ray]
                                [std::min(top - 1,
                                          fourierRawNoises[module][ray].size() - 1)];

            if (pulsars->at(i).sigma > eqt) {
                fourierGood[i] = false;
                (*pulsars)[i].dispersion = -55;
            }
        }
    }

    if (ui->fourierGoodLookingSpectresOnly->isChecked() && !ui->fourierSelectBest->isChecked()) {
        for (int i = 0; i < pulsars->size(); i++) {
            fourierGood[i] &= pulsars->at(i).snr > 0;
            if ((!fourierGood[i] && pulsars->at(i).dispersion >= 0) || pulsars->at(i).snr == -666 || pulsars->at(i).snr == -42)
                (*pulsars)[i].dispersion = -7777;
        }
    }


    for (int i = 0; i < pulsars->size(); i++) {
        if (ui->fourierPeak->isChecked())
            (*pulsars)[i].dispersion = 0;

        (*pulsars)[i].showInTable = fourierGood[i];

        if (fourierGood[i] && !pulsars->at(i).fourierDuplicate) {
            int module = pulsars->at(i).module - 1;
            int ray = pulsars->at(i).ray - 1;
            for (int k = 0; k < fourierSpectreSize; k++)
                fourierSumm[module][ray][k] += pulsars->at(i).data.data[0][0][0][k];


            if (ui->fourierPeak->isChecked() || ui->fourierSelectBest->isChecked())
                (*pulsars)[i].dispersion = (int)fourierGood[i];
        }
    }

    ui->currentStatus->setText("Generating white zone");
    progressBar->show();
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

            if (ui->noGPS->isChecked())
                destroyGPS(data);

            Pulsar pl;
            pl.module = module + 1;
            pl.ray = ray + 1;
            pl.snr = -666;
            pl.filtered = false;
            pl.data = data;
            pl.valid = true;
            pl.showInTable = true;
            pl.period = 0;
            if (pulsars->size())
                pl.nativeTime = pulsars->last().nativeTime;

            pl.findFourierData(ui->fourierPointsToSkip->value());
            pl.data.sigma = pl.firstPoint;
            whiteZone->push_front(pl);

            if (ui->fourierAllPeaks->isChecked())
                while (pl.snr > FOURIER_PULSAR_LEVEL_SNR) {
                    pl.data.fork();
                    pl.data.releaseProtected = true;
                    pl.snr = -777;
                    pl.findFourierData(pl.firstPoint);
                    pl.data.sigma = pl.firstPoint;
                    if (pl.snr > 0)
                        whiteZone->push_front(pl);
                    else {
                        pl.data.releaseProtected = false;
                        pl.data.releaseData();
                        break;
                    }
                }
        }

    for (int i = 0; i < pulsars->size(); i++)
        whiteZone->push_back(pulsars->at(i));

    delete pulsars;
    pulsars = whiteZone;

    progressBar->hide();
    pulsarsEnabled.resize(pulsars->size());
}

void Analytics::calculateCaches() {
    static bool calculating = false;
    if (calculating) {
        calculating = false;
        ui->fourierCalculateCaches->setText("Stopping...");
        ui->fourierCalculateCaches->setEnabled(false);
        return;
    } else
        calculating = true;

    ui->fourierCalculateCaches->setText("Stop calculating");
    for (int i = std::max(ui->fourierBlockNo->value(), 1); i < 424; i++) {
        qApp->processEvents();
        if (!calculating)
            break;

        ui->fourierBlockNo->setValue(i);
        if (ui->fourierSelectBest->isChecked())
            fourierSelectBestAuto();

        window->update();
        loadFourierData(true);
    }

    calculating = false;
    ui->fourierCalculateCaches->setText("Calculate caches");
    ui->fourierCalculateCaches->setEnabled(true);
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
    progressBar->show();
    int size = pulsars->size();
    for (int i = 0; i < size; i++)
        if (pulsars->at(i).filtered && pulsars->at(i).snr > FOURIER_PULSAR_LEVEL_SNR) {
            if (i % 100 == 0) {
                progressBar->setValue(100 * i / size);
                qApp->processEvents();
            }

            Pulsar pl = pulsars->at(i);

            pl.findFourierData(ui->fourierPointsToSkip->value());
            pl.data.sigma = pl.firstPoint;
            pl.fourierDuplicate = true;
            pulsars->push_back(pl);

            while (pl.snr > FOURIER_PULSAR_LEVEL_SNR) {
                pl.data.releaseProtected = true;
                pl.snr = -777;
                pl.findFourierData(pl.firstPoint + (3 + longData * 10));
                pl.data.sigma = pl.firstPoint;
                if (pl.snr > 0)
                    pulsars->push_back(pl);
            }
        }

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

    ui->knownPulsars->setChecked(false);
    ui->knownNoise->setChecked(false);
}

void Analytics::oneWindow() {
    window->setParent(NULL);
    list->setParent(NULL);
    setParent(NULL);
    QObject::disconnect(list, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
    progressBar = ui->progressBar;
    if (oneWindowMode) {
        ui->oneWindow->setText("One window");
        oneWindowMode = false;
        window->addWidgetToMainLayout(NULL, NULL, transient);
        list->show();
        show();
        QObject::connect(list, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        return;
    } else
        ui->oneWindow->setText("Gimp mode");

    oneWindowMode = true;
    progressBar = Settings::settings()->getProgressBar();

    setContentsMargins(0, 0, 0, 0);

    QObject::connect(list, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));

    window->showMaximized();
    window->addWidgetToMainLayout(this, list, transient);
    window->show();
}

void Analytics::compressLayout() {
#ifdef Q_OS_LINUX
    ui->groupBox->layout()->setSpacing(1);
    ui->groupBox_2->layout()->setSpacing(1);
    ui->groupBox_3->layout()->setSpacing(1);
    ui->groupBox_4->layout()->setSpacing(1);
    ui->groupBox_5->layout()->setSpacing(1);
    ui->groupBox_6->layout()->setSpacing(1);
    ui->groupBox_7->layout()->setSpacing(1);
    ui->widget->layout()->setSpacing(1);
    ui->widget_4->layout()->setSpacing(1);
    ui->widget_7->layout()->setSpacing(1);
    ui->widget_10->layout()->setSpacing(1);
    ui->gridLayout->setSpacing(1);
    ui->knownPulsarsAndNoises->layout()->setSpacing(1);
    layout()->setSpacing(1);
#endif
}

void Analytics::destroyPeak(Data &spectre, int point) {
    while (point < Settings::settings()->getFourierSpectreSize() - 1 &&
           spectre.data[0][0][0][point+1] > spectre.data[0][0][0][point])
        point++;

    while (point > 0 &&
           spectre.data[0][0][0][point-1] > spectre.data[0][0][0][point])
        point--;

    int start = point - 1;
    int end = point + 1;
    while (start > 0 &&
           spectre.data[0][0][0][start-1] < spectre.data[0][0][0][start])
        start--;

    while (end < Settings::settings()->getFourierSpectreSize() - 1 &&
           spectre.data[0][0][0][end+1] < spectre.data[0][0][0][end])
        end++;

    for (int i = start; i < end; i++)
        spectre.data[0][0][0][i] = spectre.data[0][0][0][start];
}

void Analytics::destroyGPS(Data &spectre) {
    if (Settings::settings()->getFourierSpectreSize() == 1024) {
        int point = 204;
        int add = point;
        while (point < Settings::settings()->getFourierSpectreSize()) {
            destroyPeak(spectre, point);
            point += add;
        }
    }

}

bool Analytics::fourierLoadNoises() {
    if (!noises.isValid()) {
        QString noisesFile = QDir(folder).absolutePath();
        if (longData)
            noisesFile += LONG_NOISES;
        else
            noisesFile += SHORT_NOISES;

        Reader reader;
        progressBar->show();
        QObject::connect(&reader, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        noises = reader.readBinaryFile(noisesFile);
        noises.releaseProtected = true;
        progressBar->hide();
        noisesHeader = Settings::settings()->getLastHeader();
        if (!noises.isValid()) {
            QMessageBox::warning(NULL, "Error: No noises files found",
                                       "There are no noises files found!\n"
                                       "You should use latest slicing. If you are using it,\n"
                                       "contact <vtyulb@vtyulb.ru> for further instructions.\n"
                                       "Noises file must me located at " + noisesFile);

            return false;
        }
    }

    return true;
}

void Analytics::fourierSelectBestAuto() {
    if (!fourierLoadNoises())
        return;

    QStringList names = noisesHeader["stairs_names"].split(",");
    int module = ui->module->value() - 1;
    int ray = ui->ray->value() - 1;
    if (module < 0 || module >= 6 || ray < 0 || ray >= 8) {
        qDebug() << "invalid module / ray";
        module = 0;
        ray = 0;
    }

    qDebug() << "select best auto on module" << module + 1 << "ray" << ray + 1;

    QString block = "/" + QString::number(ui->fourierBlockNo->value());
    QVector<double> sigmas;
    for (int i = 0; i < names.size(); i++)
        if (names[i].endsWith(block)) {
            double sigma = 0;
            for (int j = 0; j < noises.channels; j++)
                sigma += noises.data[module][j][ray][i];

            sigmas.push_back(sigma);
        }

    std::sort(sigmas.begin(), sigmas.end());
    for (int i = 0; i < 4; i++)
        sigmas[i] = sigmas.last();
    std::sort(sigmas.begin(), sigmas.end());

    int medRes = sigmas.size();
    for (int i = 0; i < sigmas.size(); i++)
        if (sigmas[i] > sigmas[sigmas.size() / 2] * 1.1) {
            medRes = i;
            break;
        }

    for (int i = sigmas.size() - 1; i >= 0; i--) {
        sigmas[i] /= sigmas[0];
        sigmas[i] *= sigmas[i];
    }

    Data res;
    res.modules = 1;
    res.rays = 2;
    res.channels = 1;
    res.npoints = sigmas.size();
    res.init();

    double max = 1;
    res.data[0][0][0][0] = sigmas[0];
    for (int i = 1; i < sigmas.size(); i++) {
        sigmas[i] += sigmas[i - 1];
        res.data[0][0][0][i] = (i + 1) / sqrt(sigmas[i]);
        if (res.data[0][0][0][i] > max) {
            max = res.data[0][0][0][i];
            res.sigma = i * 0.8;
        }
    }

    for (int i = 0; i < sigmas.size(); i++)
        res.data[0][0][1][i] = sqrt(i + 1);

    res.sigma = std::min(medRes, int(res.sigma + 0.5));
    ui->fourierBestNumber->setValue(int(res.sigma + 0.5));

    Settings::settings()->setLastHeader(QMap<QString, QString>());
    window->regenerate(res);
}

void Analytics::fourierSelectBestEnabled(bool enabled) {
    ui->fourierGoodLookingSpectresOnly->setEnabled(!enabled);
}

void Analytics::fourierShowNoises() {
    if (!fourierLoadNoises())
        return;

    QStringList originalNames = noisesHeader["stairs_names"].split(",");

    QString block = "/" + QString::number(ui->fourierBlockNo->value());
    QStringList names;
    for (int i = 0; i < originalNames.size(); i++)
        if (originalNames[i].endsWith(block))
            names.push_back(originalNames[i]);

    if (!names.size()) {
        QMessageBox::information(NULL, "Invalid block", "No data found for this block. Showing everything.");
        block = "";
        names = originalNames;
    }

    Data blockNoise = noises;
    blockNoise.npoints = names.size();
    if (block != "") {
        blockNoise.fork();
        int current = 0;
        for (int i = 0; i < originalNames.size(); i++)
            if (originalNames[i].endsWith(block)) {
                for (int module = 0; module < noises.modules; module++)
                    for (int ray = 0; ray < noises.rays; ray++)
                        for (int channel = 0; channel < noises.channels; channel++)
                            blockNoise.data[module][channel][ray][current] = std::min(noises.data[module][channel][ray][i], 50.0f);

                current++;
            }
    }

    QMap<QString, QString> newNames;
    newNames["stairs_names"] = names.join(",");
    if (ui->fourierShowNoises->text() == "Sort") {
        for (int module = 0; module < blockNoise.modules; module++)
            for (int ray = 0; ray < blockNoise.rays; ray++)
                for (int channel = 0; channel < blockNoise.channels; channel++)
                    std::sort(blockNoise.data[module][channel][ray], blockNoise.data[module][channel][ray] + blockNoise.npoints);

        ui->fourierShowNoises->setText("Noises");
    } else {
        Settings::settings()->setLastHeader(newNames);
        ui->fourierShowNoises->setText("Sort");
    }

    window->regenerate(blockNoise);
}

void Analytics::fourierShowSpectresNoise() {
    int module = std::min(max(ui->module->value() - 1, 0), 5);
    int ray = std::min(max(ui->ray->value() - 1, 0), 7);

    Data res;
    res.modules = 1;
    res.rays = 1;
    res.channels = 1;
    res.npoints = fourierNoises[module][ray].size();
    res.init();

    for (int i = 0; i < res.npoints; i++)
        res.data[0][0][0][i] = std::min(fourierNoises[module][ray][i], 20.0);

    double value = fourierNoises[module][ray][res.npoints / 2] * 1.3;
    for (int i = 0; i < res.npoints; i++)
        if (fourierNoises[module][ray][i] > value) {
            res.sigma = i;
            break;
        }

    window->regenerate(res);
}

void Analytics::transientSaveImageForPublication() {
    transientSaveImage(true);
}

void Analytics::transientSaveImage(bool forPublication) {    
    Pulsar *p = list->currentPulsar;

    QString lastPath = QSettings().value("TransientsDirectory").toString();
    lastPath += "/" + getPulsarJName(p->module, p->ray, p->nativeTime) + "___" + QUuid::createUuid().toString().replace("{", "").replace("}", "").replace("-", "").left(6) + ".png";
    QString savePath = QFileDialog::getSaveFileName(this, "Spectre & profile filename", lastPath);
    if (savePath == "")
        return;

    if (!savePath.endsWith(".png"))
        savePath += ".png";

    QImage im1 = Settings::settings()->getSpectreDrawer()->ui->drawer->spectre;
    QString tmpFile = QDir::tempPath() + "/bsa-analytics-screen.png";
    window->generateImage(tmpFile);
    QImage im2(tmpFile);

    int mn = im2.width();
    int mx = 0;
    for (int i = 0; i < im2.height(); i++)
        for (int j = 0; j < im2.width(); j++)
            if (im2.pixel(j, i) != QColor(255, 255, 255).rgb()) {
                mn = std::min(mn, j);
                mx = std::max(mx, j);
            }

    im2 = im2.copy(mn, 0, mx - mn, im2.height());
    im2 = im2.scaled(im1.width() - 50, im1.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QImage res(im1.width(), im1.height() * 2, QImage::Format_ARGB32);
    std::swap(im1, im2);

    for (int i = 0; i < im1.height(); i++)
        for (int j = 0; j < im1.width(); j++)
            res.setPixel(j + 50, i, im1.pixel(j, i));

    for (int i = 0; i < im2.height(); i++)
        for (int j = 0; j < im2.width(); j++)
            res.setPixel(j, i + im1.height(), im2.pixel(j, i));

    for (int i = 0; i < im1.height(); i++)
        for (int j = 0; j < 50; j++)
            res.setPixel(j, i, QColor(255, 255, 255).rgb());

    QString data = "Block: " + QString::number(ui->fourierBlockNo->value()) + "\n" +
                   "Impulse time: " + p->data.time.time().toString() + "\n" +
                   "Module: " + QString::number(p->module) + "\n" +
                   "Ray: " + QString::number(p->ray) + "\n" +
                   "Dispersion: " + QString::number(p->dispersion) + "\n" +
                   "Original file name: " + p->data.previousLifeName.split(" ").at(3) + "\n" +
                   "File name in block: " + p->data.previousLifeName.split(" ").at(1);

    if (forPublication)
        res = res.copy(50, 0, res.width() - 50, res.height());
    else {
        QPainter painter(&res);
        painter.drawText(QRectF(5, 20, im1.width(), im1.height()), Qt::AlignLeft, data);
        painter.end();
    }

    res.save(savePath);
    QSettings().setValue("TransientsDirectory", QFileInfo(savePath).absoluteDir().absolutePath());
    QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
}

void Analytics::buildTransientWhitezone(Pulsars &res) {
    int maxdisp = 50;
    for (int i = 0; i < res->size(); i++)
        for (int j = 0; j < res->at(i).data.npoints; j++)
            maxdisp = std::max(maxdisp, res->at(i).dispersion);

    maxdisp += 1;

    Data whitezone[6][8];
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 8; j++) {
            whitezone[i][j].modules = 1;
            whitezone[i][j].rays = 1;
            whitezone[i][j].channels = 1;
            whitezone[i][j].npoints = maxdisp;
            whitezone[i][j].init();
            whitezone[i][j].releaseProtected = true;
            for (int k = 0; k < maxdisp; k++)
                whitezone[i][j].data[0][0][0][k] = 0;
        }

    for (int i = 0; i < res->size(); i++)
        whitezone[res->at(i).module - 1][res->at(i).ray - 1].data[0][0][0][res->at(i).dispersion] += 1;

    for (int i = 5; i >= 0; i--)
        for (int j = 7; j >= 0; j--) {
            Pulsar p;
            p.module = i + 1;
            p.ray = j + 1;
            p.dispersion = 0;
            p.filtered = false;
            p.data = whitezone[i][j];
            bool good = false;
            for (int k = 0; k < maxdisp; k++)
                if (whitezone[i][j].data[0][0][0][k] > 1.5)
                    good = true;

            if (good)
                res->push_front(p);
            else {
                whitezone[i][j].releaseProtected = false;
                whitezone[i][j].releaseData();
            }
        }
}

void Analytics::enableTransientWhitezone(bool b) {
    transientWhitezoneEnabled = b;
}

Analytics::~Analytics() {
    qDebug() << "analytics destroyed";
    QSettings().setValue("GimpMode", QVariant(!oneWindowMode));
    delete ui;
}
