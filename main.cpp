#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QVector>
#include <signal.h>

namespace mainSpace {
    MainWindow *w;
    QString program;
}

void restart(int signal = 0) {
    qDebug() << "restarting";
    QMessageBox::about(mainSpace::w, "Critical error", "Wrong file format or any other critical error...");
    delete mainSpace::w;
    QProcess::startDetached(mainSpace::program);
    exit(0);
}

int main(int argc, char *argv[])
{
    qDebug() << sizeof(QVector<float>);

    signal(SIGABRT, restart);
    signal(SIGSEGV, restart);

    QApplication a(argc, argv);

    mainSpace::program = QString(argv[0]);
    mainSpace::w = new MainWindow;
    mainSpace::w->show();

    try {
        a.exec();
    } catch (...) {
        restart();
    }

    return 0;
}
