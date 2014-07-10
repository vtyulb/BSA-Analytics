#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
    QObject::connect(ui->actionAutoDraw, SIGNAL(triggered(bool)), this, SLOT(autoDraw(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(this);

    if (path != "") {
        Reader reader;
        QObject::connect(&reader, SIGNAL(progress(double)), this, SLOT(readProgressChanged(double)));
        const Data data = reader.readFile(path);
        statusBar()->showMessage("Done", 2000);
        if (data.size()) {
            ui->label->hide();
            delete ui->frame->layout();
            QHBoxLayout *layout = new QHBoxLayout(ui->frame);
            drawer = new Drawer(data, this);
            layout->addWidget(drawer);
            ui->frame->setLayout(layout);
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
