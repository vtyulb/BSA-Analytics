#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStackedLayout>
#include <QSettings>
#include <QTimer>
#include <QMenu>
#include <QProcess>
#include <QDesktopServices>
#include <QStyleFactory>
#include <QApplication>
#include <QThread>

#include <customopendialog.h>
#include <datagenerator.h>
#include <pulsarlist.h>
#include <precisesearchgui.h>
#include <precisetiming.h>
#include <flowdetecter.h>
#include <settings.h>
#include <updater.h>

MainWindow::MainWindow(QString file, QWidget *parent) :
    QMainWindow(NULL),
    ui(new Ui::MainWindow),
    drawer(NULL),
    fileToOpen(file)
{
    ui->setupUi(this);
    generateStyles();

    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    QObject::connect(ui->actionOpen_Binary, SIGNAL(triggered()), this, SLOT(openBinaryFile()));
    QObject::connect(ui->actionCustom_open, SIGNAL(triggered()), this, SLOT(customOpen()));
    QObject::connect(ui->actionSlideshow, SIGNAL(triggered()), this, SLOT(runSlideshow()));
    QObject::connect(ui->actionPulsar_searcher, SIGNAL(triggered()), this, SLOT(openPulsarFile()));
    QObject::connect(ui->actionPulsar_analytics, SIGNAL(triggered()), this, SLOT(openAnalytics()));
    QObject::connect(ui->actionPulsar_analytics_low_memory, SIGNAL(triggered(bool)), this, SLOT(openAnalytics(bool)));
    QObject::connect(ui->actionPulsar_fourier_analytics, SIGNAL(triggered()), this, SLOT(openFourierAnalytics()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
    QObject::connect(ui->actionExport_to_CSV, SIGNAL(triggered()), this, SLOT(runCSVexport()));
    QObject::connect(ui->actionAutoDraw, SIGNAL(triggered(bool)), this, SLOT(autoDraw(bool)));
    QObject::connect(ui->actionAxes, SIGNAL(triggered(bool)), this, SLOT(drawAxes(bool)));
    QObject::connect(ui->actionNull_on_OY_axe, SIGNAL(triggered(bool)), this, SLOT(drawNullOnOYaxis(bool)));
    QObject::connect(ui->actionNet, SIGNAL(triggered(bool)), this, SLOT(drawNet(bool)));
    QObject::connect(ui->actionFast, SIGNAL(triggered(bool)), this, SLOT(drawFast(bool)));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    QObject::connect(ui->actionAbout_Qt, SIGNAL(triggered()), this, SLOT(showAboutQt()));
    QObject::connect(ui->actionClone, SIGNAL(triggered()), this, SLOT(clone()));
    QObject::connect(ui->actionLive, SIGNAL(triggered(bool)), this, SLOT(drawLive(bool)));
    QObject::connect(ui->actionPrecise_search, SIGNAL(triggered()), this, SLOT(runPreciseGui()));
    QObject::connect(ui->actionPrecise_timing, SIGNAL(triggered()), this, SLOT(runPreciseTimingGui()));
    QObject::connect(ui->actionSound_mode, SIGNAL(triggered()), this, SLOT(soundModeTriggered()));
    QObject::connect(ui->actionRotation_Measure, SIGNAL(triggered()), this, SLOT(setRotationMeasureMode()));
    QObject::connect(ui->actionFlux_Density, SIGNAL(triggered()), this, SLOT(setFluxDensityMode()));
    QObject::connect(ui->actionNormalize_data, SIGNAL(triggered()), this, SLOT(normalizeData()));
    QObject::connect(ui->actionHandBook, SIGNAL(triggered()), this, SLOT(showHelp()));
    QObject::connect(ui->actionCheck_for_updates, SIGNAL(triggered()), this, SLOT(checkForUpdatesStatusChanged()));
    QObject::connect(ui->actionStable, SIGNAL(triggered()), this, SLOT(switchToStableChannel()));
    QObject::connect(ui->actionNightly, SIGNAL(triggered()), this, SLOT(switchToNightlyChannel()));

    progress = new QProgressBar(this);
    progress->setRange(0, 100);

    Settings::settings()->setMainWindow(this);
    Settings::settings()->setProgressBar(progress);
    setAttribute(Qt::WA_DeleteOnClose);

    statusBar()->addWidget(progress, 1);
    ui->actionAxes->setChecked(true);
    loadSettings();

    if (!parent) {
        if (file != "")
            QTimer::singleShot(400, this, SLOT(openStartFile()));
        else {
            QTimer::singleShot(400, this, SLOT(customOpen()));
            if (ui->actionStartup_message->isChecked()) {
                Data startupMessage = DataGenerator::generateRandomPhrase();
                regenerate(startupMessage);
            }
        }
    }

    static Updater updater;
    QObject::connect(ui->actionUpdate, SIGNAL(triggered(bool)), &updater, SLOT(checkForUpdates(bool)));
    if (ui->actionCheck_for_updates->isChecked())
        updater.checkForUpdates(true);
}

MainWindow::~MainWindow() {
    qDebug() << "MainWindow destroyed";
    saveSettings();
    delete ui;
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(this, "Opening RK8 BSA1/3 (text data)", lastOpenPath);

    if (path == "")
        return;

    nativeOpenFile(path);
}

void MainWindow::openBinaryFile() {
    QString path = QFileDialog::getOpenFileName(this, "Opening Binary BSA3", lastOpenPath);

    if (path == "")
        return;

    nativeOpenFile(path, 0, 0, QDateTime(), true);
}

void MainWindow::openStartFile() {
    nativeOpenFile(fileToOpen, 0, 2, QDateTime(), true);
    QDir::setCurrent(qApp->applicationDirPath());
}

void MainWindow::openAnalytics(bool hasMemory, bool fourier) {
    QString path = QFileDialog::getExistingDirectory(this, "Analytics folder", lastOpenPath);
    if (path == "")
        return;

    QStringList l;
    l << "--analytics" << path;
    if (!hasMemory)
        l << "--low-memory";

    if (fourier)
        l << "--fourier";

    QProcess::startDetached(qApp->arguments()[0], l);
    qApp->exit(0);
}

void MainWindow::openFourierAnalytics() {
    openAnalytics(true, true);
}

void MainWindow::openPulsarFile() {
    QString path = QFileDialog::getOpenFileName(this, "Opening *.pulsar file", lastOpenPath, "Pulsar files (*.pulsar)");

    if (path == "")
        return;

    decodeLastPath(path);

    QStringList l;
    l << "--analytics" << path;
    QProcess::startDetached(qApp->arguments()[0], l);
    qApp->exit(0);
}

void MainWindow::runSlideshow() {
    QString path = QFileDialog::getExistingDirectory(this, "Directory with binary data", lastOpenPath);
    if (path == "")
        return;

    decodeLastPath(path);
    QDir dir(path);
    QFileInfoList l = dir.entryInfoList(QDir::Files);
    for (int i = 0; i < l.size(); i++) {
        qApp->processEvents();
        QString name = l[i].absoluteFilePath();
        Reader reader;
        QObject::connect(&reader, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
        Data res = reader.readBinaryFile(name);
        regenerate(res);
        QThread::sleep(1);
    }
}

QString MainWindow::nativeDecodeLastPath(QString path) {
    for (int i = path.length() - 1; i; i--)
        if (path[i] == '/' || path[i] == '\\')
            return path.left(i);

    return QDir::homePath();
}

void MainWindow::decodeLastPath(QString path) {
    lastOpenPath = nativeDecodeLastPath(path);
}

void MainWindow::nativeOpenFile(QString fileName, int skip, int skipFirstRay, QDateTime time, bool binary) {
    decodeLastPath(fileName);
    Reader reader;
    progress->show();
    QObject::connect(&reader, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
    Data data = reader.readFile(fileName, skip, skipFirstRay, time, binary);
    statusBar()->showMessage("Done", 2000);
    if (data.npoints)
        regenerate(data);
}

void MainWindow::regenerate(Data &data) {
    show();
    if (ui->actionFlux_Density->isChecked() || ui->actionRotation_Measure->isChecked())
        normalizeData();

    delete ui->label; ui->label = NULL;
    if (drawer) {
        Data *old = &drawer->drawer->data;
        if (old->modules == data.modules && old->rays == data.rays && old->channels == data.channels) {
            drawer->pushNewData(data);
            return;
        }

        qDebug() << "resetting drawer";
        QLayout *layout = drawer->parentWidget()->layout();
        delete drawer;
        QWidget *item = NULL;
        if (layout->count() == 1)
            item = layout->itemAt(0)->widget();

        drawer = new Drawer(data, this);
        layout->addWidget(drawer);
        if (item)
            layout->addWidget(item);
    } else {
        drawer = new Drawer(data, this);
        ui->centralWidget->layout()->addWidget(drawer);
    }

    QObject::connect(drawer->drawer, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
    QObject::connect(ui->actionPrint, SIGNAL(triggered()), drawer->drawer, SLOT(print()));

    autoDraw(ui->actionAutoDraw->isChecked());
    drawAxes(ui->actionAxes->isChecked());
    drawNet(ui->actionNet->isChecked());
    drawFast(ui->actionFast->isChecked());
    drawLive(ui->actionLive->isChecked());
}

void MainWindow::readProgressChanged(double progress) {
    statusBar()->clearMessage();
    statusBar()->showMessage(QString("Readed %1%").arg(QString::number(int(progress * 100))));
}

void MainWindow::saveFile() {
    QString path = QFileDialog::getSaveFileName(this, "", "", "Image *.png;;Vector Image *.svg");

    if (path != "")
        drawer->saveFile(path);
}

void MainWindow::runCSVexport() {
    if (drawer)
        drawer->drawer->exportDataToCSV();
}

void MainWindow::autoDraw(bool b) {
    if (drawer)
        drawer->drawer->autoDrawing = b;
}

void MainWindow::drawAxes(bool b) {
    if (drawer)
        drawer->drawer->drawAxesFlag = b;
}

void MainWindow::drawNullOnOYaxis(bool b) {
    Settings::settings()->setNullOnOYaxis(b);
}

void MainWindow::drawNet(bool b) {
    if (drawer)
        drawer->drawer->drawNet = b;
}

void MainWindow::drawLive(bool b) {
    if (drawer)
        drawer->drawer->live = b;
}

void MainWindow::drawFast(bool b) {
    if (drawer)
        drawer->drawer->drawFast = b;
}

void MainWindow::showAboutQt() {
    QMessageBox::aboutQt(this);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About", "Program was written specially\n"
                                      "for S.A.Tyulbashev <serg@prao.ru>\n"
                                      "by V.S.Tyulbashev <vtyulb@vtyulb.ru>\n\n" +
                                      Settings::settings()->version());
}

void MainWindow::saveSettings() {
    qDebug() << "saving global settings";
    QSettings s;
    if (!isMaximized())
        s.setValue("geometry", QVariant(saveGeometry()));

    s.setValue("autoDraw", QVariant(ui->actionAutoDraw->isChecked()));
    s.setValue("openPath", QVariant(lastOpenPath));
    s.setValue("fast", QVariant(ui->actionFast->isChecked()));
    s.setValue("live", QVariant(ui->actionLive->isChecked()));
    s.setValue("DrawNullOnOYaxis", QVariant(ui->actionNull_on_OY_axe->isChecked()));
    s.setValue("CheckForUpdates", QVariant(ui->actionCheck_for_updates->isChecked()));
    s.setValue("StartupMessage", QVariant(ui->actionStartup_message->isChecked()));
}

void MainWindow::loadSettings() {
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());
    ui->actionAutoDraw->setChecked(s.value("autoDraw", true).toBool());
    ui->actionFast->setChecked(s.value("fast", false).toBool());
    ui->actionLive->setChecked(s.value("live", true).toBool());
    ui->actionNull_on_OY_axe->setChecked(s.value("DrawNullOnOYaxis", true).toBool());
    Settings::settings()->setNullOnOYaxis(ui->actionNull_on_OY_axe->isChecked());
    ui->actionStartup_message->setChecked(s.value("StartupMessage", true).toBool());
    lastOpenPath = s.value("openPath").toString();
    ui->actionCheck_for_updates->setChecked(s.value("CheckForUpdates", true).toBool());
    if (s.value("Stable", STABLE_VERSION).toBool())
        ui->actionStable->setChecked(true);
    else
        ui->actionNightly->setChecked(true);
}

void MainWindow::customOpen() {
    CustomOpenDialog *dialog = new CustomOpenDialog(lastOpenPath, this);
    QObject::connect(dialog, SIGNAL(customOpen(QString, int, int, QDateTime, bool)), this, SLOT(nativeOpenFile(QString, int, int, QDateTime, bool)));
    dialog->show();
}

void MainWindow::clone() {
    customOpen();

    qApp->processEvents();

    drawer->setParent(NULL);
    drawer->setWindowFlags(Qt::SubWindow);
    drawer->show();
    autoDraw(false);

    drawer = NULL;
}

void MainWindow::runPreciseGui() {
    static PreciseSearchGui *gui = new PreciseSearchGui(this);
    gui->show();
}

void MainWindow::runPreciseTimingGui() {
    static PreciseTiming *gui = new PreciseTiming(this);
    gui->show();
}

void MainWindow::soundModeTriggered() {
    Settings::settings()->setSoundMode(ui->actionSound_mode->isChecked());
}

void MainWindow::setRotationMeasureMode() {
    ui->actionFlux_Density->setChecked(false);
    if (ui->actionRotation_Measure->isChecked()) {
        Settings::settings()->loadStair();
        Settings::settings()->setSourceMode(RotationMeasure);
        normalizeData();
    } else
        Settings::settings()->setSourceMode(NoSourceMode);
}

void MainWindow::setFluxDensityMode() {
    ui->actionRotation_Measure->setChecked(false);
    if (ui->actionFlux_Density->isChecked()) {
        Settings::settings()->loadStair();
        Settings::settings()->setSourceMode(FluxDensity);
        normalizeData();
    } else
        Settings::settings()->setSourceMode(NoSourceMode);
}

void MainWindow::normalizeData() {
    static Data last;
    if (Settings::settings()->getLastData().isValid() && last.name != Settings::settings()->getLastData().name) {
        last = Settings::settings()->getLastData();
        if (!Settings::settings()->loadStair())
            QMessageBox::warning(this, "Error", "Stairs are too far from this data. Normalization is not completed!");
        else {
            qDebug() << "normalizing data";
            progress->show();
            for (int module = 0; module < last.modules; module++)
                for (int channel = 0; channel < last.channels; channel++) {
                    progress->setValue((module * last.channels + channel + 1) * 100 / (last.modules * last.channels));
                    qApp->processEvents();
                    for (int ray = 0; ray < last.rays; ray++)
                        for (int i = 0; i < last.npoints; i++)
                            last.data[module][channel][ray][i] /= Settings::settings()->getStairHeight(module, ray, channel) / 2100.0;
                }

            for (int module = 0; module < last.modules; module++)
                for (int ray = 0; ray < last.rays; ray++)
                    for (int i = 0; i < last.npoints; i++) {
                        last.data[module][last.channels - 1][ray][i] = 0;
                        for (int channel = 0; channel < last.channels - 1; channel++)
                            last.data[module][last.channels - 1][ray][i] += last.data[module][channel][ray][i] / (last.channels - 1);
                    }

            progress->hide();
            if (drawer)
                drawer->drawer->resetVisibleRectangle();
        }
    }
}

void MainWindow::showHelp() {
    const QUrl help = QUrl::fromLocalFile(DOC_PATH + "/HandBook.pdf");
    qDebug() << "opening help" << help;
    QDesktopServices::openUrl(help);
}

void MainWindow::addWidgetToMainLayout(QWidget *w1, QWidget *w2, bool addSpectre) {
    if (w1 == NULL)
        return;

    qDebug() << "messing up with mainwindow layout";

    ui->centralWidget->layout()->removeWidget(drawer);

    ui->centralWidget->layout()->addWidget(w1);
    ui->centralWidget->layout()->addWidget(w2);
    if (drawer) {
        QWidget *rightSide = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(rightSide);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(drawer);
        if (addSpectre) {
            qDebug() << "spectre drawer created";
            SpectreDrawer *spectreDrawer = new SpectreDrawer;
            spectreDrawer->setMinimumHeight(500);
            spectreDrawer->setMaximumHeight(500);
            Settings::settings()->setSpectreDrawer(spectreDrawer);
            layout->addWidget(spectreDrawer);
            spectreDrawer->hide();
        }



        ui->centralWidget->layout()->addWidget(rightSide);
    }
}

void MainWindow::generateStyles() {
    styles = new QMenu("Style");
    QStringList styleNames = QStyleFactory::keys();

#ifdef WIN32
    qApp->setStyle(QSettings().value("Style", "Fusion").toString());
#else
    qApp->setStyle(QSettings().value("Style").toString());
#endif

    for (int i = 0; i < styleNames.size(); i++) {
        styles->addAction(new QAction(styleNames[i], this));
        styles->actions().last()->setCheckable(true);
#ifdef WIN32
        if (styleNames[i] == "Fusion") {
            styles->actions().last()->setText("Fusion (default)");
        }
#endif
        if (styleNames[i].toUpper() == qApp->style()->objectName().toUpper())
            styles->actions().last()->setChecked(true);
    }

    qDebug() << qApp->style()->objectName();

    QObject::connect(styles, SIGNAL(triggered(QAction*)), this, SLOT(setStyle(QAction*)));
    ui->menuEdit->addMenu(styles);
}

void MainWindow::setStyle(QAction *action) {
    QString style = action->text().replace("&", "").replace(" (default)", "");
    qDebug() << "setting style to" << style;
    qApp->setStyle(QStyleFactory::create(style));
    QSettings().setValue("Style", style);

    for (int i = 0; i < styles->actions().size(); i++)
        styles->actions()[i]->setChecked(false);

    action->setChecked(true);
}

void MainWindow::checkForUpdatesStatusChanged() {
    QSettings().setValue("CheckForUpdates", ui->actionCheck_for_updates->isChecked());
}

void MainWindow::switchToNightlyChannel() {
    ui->actionStable->setChecked(false);
    ui->actionNightly->setChecked(true);
    QSettings().setValue("Stable", QVariant(false));
}

void MainWindow::switchToStableChannel() {
    ui->actionNightly->setChecked(false);
    ui->actionStable->setChecked(true);
    QSettings().setValue("Stable", QVariant(true));
}

void MainWindow::generateImage(QString path) {
    drawer->drawer->drawAxesFlag = false;
    drawer->drawer->resetVisibleRectangle();
    drawer->saveFile(path);
    drawer->drawer->drawAxesFlag = ui->actionAxes->isChecked();
    drawer->drawer->resetVisibleRectangle();
}
