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

#include <customopendialog.h>
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
    QObject::connect(ui->actionPulsar_searcher, SIGNAL(triggered()), this, SLOT(openPulsarFile()));
    QObject::connect(ui->actionPulsar_analytics, SIGNAL(triggered()), this, SLOT(openAnalytics()));
    QObject::connect(ui->actionPulsar_analytics_low_memory, SIGNAL(triggered(bool)), this, SLOT(openAnalytics(bool)));
    QObject::connect(ui->actionPulsar_fourier_analytics, SIGNAL(triggered()), this, SLOT(openFourierAnalytics()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
    QObject::connect(ui->actionAutoDraw, SIGNAL(triggered(bool)), this, SLOT(autoDraw(bool)));
    QObject::connect(ui->actionAxes, SIGNAL(triggered(bool)), this, SLOT(drawAxes(bool)));
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
    QObject::connect(ui->actionSet_stair, SIGNAL(triggered()), this, SLOT(setStair()));
    QObject::connect(ui->actionHandBook, SIGNAL(triggered()), this, SLOT(showHelp()));
    QObject::connect(ui->actionCheck_for_updates, SIGNAL(triggered()), this, SLOT(checkForUpdatesStatusChanged()));

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
        else
            QTimer::singleShot(400, this, SLOT(customOpen()));
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
    delete ui->label; ui->label = NULL;
    if (drawer) {
        Data *old = &drawer->drawer->data;
        if (old->modules == data.modules && old->rays == data.rays && old->channels == data.channels) {
            drawer->pushNewData(data);
            return;
        }
    }
    delete drawer;
    drawer = new Drawer(data, this);
    ui->centralWidget->layout()->addWidget(drawer);

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
    QString path = QFileDialog::getSaveFileName(this);

    if (path != "")
        drawer->saveFile(path);
}

void MainWindow::autoDraw(bool b) {
    if (drawer)
        drawer->drawer->autoDrawing = b;
}

void MainWindow::drawAxes(bool b) {
    if (drawer)
        drawer->drawer->drawAxesFlag = b;
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
                                      "by V.S.Tyulbashev <vtyulb@vtyulb.ru>\n\n"
                                      "This version compiled\n" +
                                      QFileInfo(qApp->arguments().first()).lastModified().toString());
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
    s.setValue("CheckForUpdates", QVariant(ui->actionCheck_for_updates->isChecked()));
}

void MainWindow::loadSettings() {
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());
    ui->actionAutoDraw->setChecked(s.value("autoDraw", true).toBool());
    ui->actionFast->setChecked(s.value("fast", false).toBool());
    ui->actionLive->setChecked(s.value("live", true).toBool());
    lastOpenPath = s.value("openPath").toString();
    ui->actionCheck_for_updates->setChecked(s.value("CheckForUpdates", true).toBool());
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
    Settings::settings()->setStairStatus(NoStair);
    ui->actionFlux_Density->setChecked(false);
    if (ui->actionRotation_Measure->isChecked()) {
        if (!Settings::settings()->loadStair())
            setStair();

        Settings::settings()->setSourceMode(RotationMeasure);
    }
}

void MainWindow::setFluxDensityMode() {
    Settings::settings()->setStairStatus(NoStair);
    ui->actionRotation_Measure->setChecked(false);
    if (ui->actionFlux_Density->isChecked()) {
        if (!Settings::settings()->loadStair())
            setStair();

        Settings::settings()->setSourceMode(FluxDensity);
    }
}

void MainWindow::setStair() {
    Settings::settings()->setStairStatus(NoStair);
    Settings::settings()->setStairStatus(SettingStair);
    QMessageBox::information(this, "Setting stair",
                                   "Open file with the stair, make a rectangular around it.\n"
                                   "Rectangular height does not matter, only width and position");
}

void MainWindow::showHelp() {
    const QUrl help = QUrl::fromLocalFile(DOCPATH + "/HandBook.pdf");
    qDebug() << "opening help" << help;
    QDesktopServices::openUrl(help);
}

void MainWindow::addWidgetToMainLayout(QWidget *w1, QWidget *w2) {
    if (w1 == NULL)
        return;

    ui->centralWidget->layout()->removeWidget(drawer);
    ui->centralWidget->layout()->addWidget(w1);
    ui->centralWidget->layout()->addWidget(w2);
    if (drawer)
        ui->centralWidget->layout()->addWidget(drawer);
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
