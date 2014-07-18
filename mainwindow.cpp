#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <customopendialog.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    drawer(NULL)
{
    ui->setupUi(this);

    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    QObject::connect(ui->actionOpen_Binary, SIGNAL(triggered()), this, SLOT(openBinaryFile()));
    QObject::connect(ui->actionCustom_open, SIGNAL(triggered()), this, SLOT(customOpen()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
    QObject::connect(ui->actionAutoDraw, SIGNAL(triggered(bool)), this, SLOT(autoDraw(bool)));
    QObject::connect(ui->actionAxes, SIGNAL(triggered(bool)), this, SLOT(drawAxes(bool)));
    QObject::connect(ui->actionNet, SIGNAL(triggered(bool)), this, SLOT(drawNet(bool)));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    QObject::connect(ui->actionAbout_Qt, SIGNAL(triggered()), this, SLOT(showAboutQt()));

    progress = new QProgressBar(this);
    progress->setRange(0, 100);
    statusBar()->addPermanentWidget(progress);
    ui->actionAxes->setChecked(true);
    loadSettings();
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

    decodeLastPath(path);
    nativeOpenFile(path);
}

void MainWindow::openBinaryFile() {
    QString path = QFileDialog::getOpenFileName(this, "void", lastOpenPath);

    if (path == "")
        return;

    decodeLastPath(path);
    nativeOpenFile(path, 0, 0, true);
}

void MainWindow::decodeLastPath(QString path) {
    for (int i = path.length() - 1; i; i--)
        if (path[i] == '/' || path[i] == '\\') {
            lastOpenPath = path.left(i);
            break;
        }
}

void MainWindow::nativeOpenFile(QString fileName, int skip, int skipFirstRay, bool binary) {
    Reader reader;
    QObject::connect(&reader, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
    const Data data = reader.readFile(fileName, skip, skipFirstRay, binary);
    statusBar()->showMessage("Done", 2000);
    if (data.size()) {
        ui->label->hide();
        delete drawer;
        drawer = new Drawer(data, this);
        ui->scrollAreaWidgetContents->layout()->addWidget(drawer);

        QObject::connect(drawer->drawer, SIGNAL(progress(int)), progress, SLOT(setValue(int)));

        autoDraw(ui->actionAutoDraw->isChecked());
        drawAxes(ui->actionAxes->isChecked());
        drawNet(ui->actionNet->isChecked());
    }
}

void MainWindow::readProgressChanged(double progress) {
    statusBar()->clearMessage();
    statusBar()->showMessage(QString("Readed %1%").arg(QString::number(int(progress * 100))));
}

void MainWindow::saveFile() {
    QString path = QFileDialog::getSaveFileName(this);

    if (path != "") {
        if (path.indexOf('c') == -1)
            path += ".png";

        drawer->saveFile(path);
    }
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

void MainWindow::showAboutQt() {
    QMessageBox::aboutQt(this);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About", "Written specially for PRAO\nby V.S.Tyulbashev\n<vtyulb@vtyulb.ru>");
}

void MainWindow::saveSettings() {
    QSettings s("settings.ini", QSettings::IniFormat);
    s.setValue("geometry", QVariant(saveGeometry()));
    s.setValue("autoDraw", QVariant(ui->actionAutoDraw->isChecked()));
    s.setValue("openPath", QVariant(lastOpenPath));
}

void MainWindow::loadSettings() {
    QSettings s("settings.ini", QSettings::IniFormat);
    restoreGeometry(s.value("geometry").toByteArray());
    ui->actionAutoDraw->setChecked(s.value("autoDraw", true).toBool());
    lastOpenPath = s.value("openPath").toString();
}

void MainWindow::customOpen() {
    CustomOpenDialog *dialog = new CustomOpenDialog(this);
    QObject::connect(dialog, SIGNAL(customOpen(QString, int, int)), this, SLOT(nativeOpenFile(QString, int, int)));
    dialog->show();
}
