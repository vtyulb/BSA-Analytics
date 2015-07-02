#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QProcess>

#include <customopendialog.h>
#include <pulsarlist.h>
#include <precisesearchgui.h>
#include <precisetiming.h>
#include <settings.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    drawer(NULL)
{
    ui->setupUi(this);

    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    QObject::connect(ui->actionOpen_Binary, SIGNAL(triggered()), this, SLOT(openBinaryFile()));
    QObject::connect(ui->actionCustom_open, SIGNAL(triggered()), this, SLOT(customOpen()));
    QObject::connect(ui->actionPulsar_searcher, SIGNAL(triggered()), this, SLOT(openPulsarFile()));
    QObject::connect(ui->actionPulsar_analytics, SIGNAL(triggered()), this, SLOT(openAnalytics()));
    QObject::connect(ui->actionPulsar_analytics_low_memory, SIGNAL(triggered(bool)), this, SLOT(openAnalytics(bool)));
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

    progress = new QProgressBar(this);
    progress->setRange(0, 100);
    statusBar()->addPermanentWidget(progress);
    ui->actionAxes->setChecked(true);
    loadSettings();

    if (!parent)
        QTimer::singleShot(400, this, SLOT(customOpen()));
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(this, "void", lastOpenPath);

    if (path == "")
        return;

    nativeOpenFile(path);
}

void MainWindow::openBinaryFile() {
    QString path = QFileDialog::getOpenFileName(this, "void", lastOpenPath);

    if (path == "")
        return;

    nativeOpenFile(path, 0, 0, QDateTime(), true);
}

void MainWindow::openAnalytics(bool hasMemory) {
    QString path = QFileDialog::getExistingDirectory(this, "analytics folder", lastOpenPath);
    if (path == "")
        return;

    QStringList l;
    l << "--analytics" << path;
    if (!hasMemory)
        l << "--low-memory";

    QProcess::startDetached(qApp->arguments()[0], l);
    qApp->exit(0);
}

void MainWindow::openPulsarFile() {
    QString path = QFileDialog::getOpenFileName(this, "void", lastOpenPath, "Pulsar files (*.pulsar)");

    if (path == "")
        return;

    decodeLastPath(path);

    QStringList l;
    l << "--analytics" << path;
    QProcess::startDetached(qApp->arguments()[0], l);
    qApp->exit(0);

//    PulsarList *list = new PulsarList(path, NULL, this);
//    QObject::connect(list, SIGNAL(switchData(Data&)), this, SLOT(regenerate(Data&)));
//    QObject::connect(this, SIGNAL(destroyed()), list, SLOT(deleteLater()));
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
    QObject::connect(&reader, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
    Data data = reader.readFile(fileName, skip, skipFirstRay, time, binary);
    statusBar()->showMessage("Done", 2000);
    if (data.npoints)
        regenerate(data);
}

void MainWindow::regenerate(Data &data) {
    show();
    ui->label->hide();
    delete drawer;
    drawer = new Drawer(data, this);
    ui->scrollAreaWidgetContents->layout()->addWidget(drawer);

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
    QMessageBox::about(this, "About", "Written specially for PRAO\nby V.S.Tyulbashev\n<vtyulb@vtyulb.ru>");
}

void MainWindow::saveSettings() {
    QSettings s;
    s.setValue("geometry", QVariant(saveGeometry()));
    s.setValue("autoDraw", QVariant(ui->actionAutoDraw->isChecked()));
    s.setValue("openPath", QVariant(lastOpenPath));
    s.setValue("fast", QVariant(ui->actionFast->isChecked()));
    s.setValue("live", QVariant(ui->actionLive->isChecked()));
}

void MainWindow::loadSettings() {
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());
    ui->actionAutoDraw->setChecked(s.value("autoDraw", true).toBool());
    ui->actionFast->setChecked(s.value("fast", false).toBool());
    ui->actionLive->setChecked(s.value("live", true).toBool());
    lastOpenPath = s.value("openPath").toString();
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
