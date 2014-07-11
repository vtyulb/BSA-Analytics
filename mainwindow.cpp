#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
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
    QString path = QFileDialog::getOpenFileName(this);

    if (path != "") {
        Reader reader;
        QObject::connect(&reader, SIGNAL(progress(int)), progress, SLOT(setValue(int)));
        const Data data = reader.readFile(path);
        statusBar()->showMessage("Done", 2000);
        if (data.size()) {
            ui->label->hide();
            delete ui->frame->layout();
            QHBoxLayout *layout = new QHBoxLayout(ui->frame);
            drawer = new Drawer(data, this);
            layout->addWidget(drawer);
            ui->frame->setLayout(layout);

            QObject::connect(drawer->drawer, SIGNAL(progress(int)), progress, SLOT(setValue(int)));

            autoDraw(ui->actionAutoDraw->isChecked());
            drawAxes(ui->actionAxes->isChecked());
            drawNet(ui->actionNet->isChecked());
        }
    }
}

void MainWindow::readProgressChanged(double progress) {
    statusBar()->clearMessage();
    statusBar()->showMessage(QString("Readed %1%").arg(QString::number(int(progress * 100))));
}

void MainWindow::saveFile() {
    QString path = QFileDialog::getSaveFileName(this);

    if (path != "") {
        drawer->saveFile(path);
    }
}

void MainWindow::autoDraw(bool b) {
    drawer->drawer->autoDrawing = b;
}

void MainWindow::drawAxes(bool b) {
    drawer->drawer->drawAxesFlag = b;
}

void MainWindow::drawNet(bool b) {
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
}

void MainWindow::loadSettings() {
    QSettings s("settings.ini", QSettings::IniFormat);
    restoreGeometry(s.value("geometry").toByteArray());
    ui->actionAutoDraw->setChecked(s.value("autoDraw", true).toBool());
}
