#include <mainwindow.h>
#include <pulsarsearcher.h>
#include <QApplication>
#include <QCoreApplication>
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
    QProcess::startDetached(mainSpace::program);
    exit(0);
}

int pulsarEngine(int argc, char **argv) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("\t-h --help  for this message\n");
        printf("\t--pulsar-search /path/to/daily/data\n");
        printf("\nWritten by Vladislav Tyulbashev.\n");
        printf("About any errors please write to <vtyulb@vtyulb.ru>\n");
        return 0;
    }

    if (strcmp(argv[1], "--pulsar-search") == 0) {
        QCoreApplication a(argc, argv);
        a.setOrganizationDomain("bsa.vtyulb.ru");
        a.setOrganizationName("vtyulb");
        a.setApplicationName("BSA-Analytics");

        PulsarSearcher searcher(QString::fromUtf8(argv[2]));
        searcher.start();
        return a.exec();
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        return pulsarEngine(argc, argv);

    signal(SIGABRT, restart);
    signal(SIGSEGV, restart);

    QApplication a(argc, argv);
    a.setApplicationName("BSA-Analytics");
    a.setOrganizationName("vtyulb");
    a.setOrganizationDomain("bsa.vtyulb.ru");

    mainSpace::program = QString(argv[0]);
    mainSpace::w = new MainWindow;
    mainSpace::w->show();

    try {
        a.exec();
        delete mainSpace::w;
    } catch (...) {
        restart();
    }

    return 0;
}
