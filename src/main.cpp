#include <mainwindow.h>
#include <pulsarsearcher.h>
#include <pulsarreader.h>
#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QVector>
#include <QDir>
#include <QTime>

#include <signal.h>
#include <settings.h>
#include <analytics.h>
#include <calculationpool.h>
#include <filecompressor.h>

#include <sys/unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#ifdef Q_OS_LINUX
    #include <execinfo.h>
#endif

namespace mainSpace {
    MainWindow *w;
    QString program;
}

void restart(int signal = 0) {
    if (signal == SIGTERM)
        exit(0); // to avoid warnings

    qDebug() << "restarting";
    QMessageBox::about(mainSpace::w, "Critical error", "Wrong file format or any other critical error...");
    QProcess::startDetached(mainSpace::program);
    exit(0);
}

void catchSigSegv(int signal) {

#ifdef Q_OS_LINUX
    void *callstack[128];
    int frames = backtrace(callstack, 128);
    char **strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; i++)
        fprintf(stderr, "%d: %s\n", i, strs[i]);
#endif

    fprintf(stderr, "dead end\n");
    exit(2);
}

void pulsarEngine(int argc, char **argv) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("Usage:\nBSA-Analytics\n");
        printf("BSA-Analytics --pulsar-search <string> --save-path <string> [--threads <int>] [--skip <int>]\n");
        printf("BSA-Analytics --analytics [path-to-data] [--low-memory]\n");
        printf("BSA-Analytics --source-range <file name> <point>\n");
        printf("BSA-Analytics --sound-mode\n");
        printf("BSA-Analytics --compress <dir>\n");
        printf("BSA-Analytics --precise-pulsar-search <file name> --module <int> --ray <int> --period <double> --time <09:01:00>\n");
        printf("\nOptions:\n");
        printf("\t-h --help  for this message\n");
        printf("\t--pulsar-search /path/to/daily/data\n");
        printf("\t--save-path /path/to/save\n");
        printf("\t--threads <int> number of effective threads\n");
        printf("\t--skip <int> for skipping first N files\n");
        printf("\t--no-filter for disabling filter\n");
        printf("\t--sub-zero for output pulsars with snr 2-5 (only good)\n");
        printf("\t--source-range <file name> <point> for running in a special mode\n");
        printf("\t--sound-mode for listening data\n");
        printf("\t--analytics to run in analytics mode\n");
        printf("\t--low-memory to do not save data roads in analytics mode\n");
        printf("\nWritten by Vladislav Tyulbashev.\n");
        printf("About any errors please write to <vtyulb@vtyulb.ru>\n");
        exit(0);
    }

    QString savePath;
    QString dataPath;
    QString analyticsPath;
    int threads = -1;
    bool analytics = false;

    bool preciseSearch = false;
    int module = 1;
    int ray = 1;
    double period = 1;
    QTime time(0, 0, 0);

    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--pulsar-search") == 0)
            dataPath = QString::fromUtf8(argv[i + 1]);
        else if (strcmp(argv[i], "--save-path") == 0)
            savePath = QString::fromUtf8(argv[i + 1]);
        else if (strcmp(argv[i], "--threads") == 0)
            threads = QString::fromUtf8(argv[i + 1]).toInt();
        else if (strcmp(argv[i], "--no-filter") == 0)
            Settings::settings()->setIntellectualFilter(false);
        else if (strcmp(argv[i], "--skip") == 0)
            Settings::settings()->setSkipCount(QString(argv[i + 1]).toInt());
        else if (strcmp(argv[i], "--sub-zero") == 0)
            Settings::settings()->setSubZero(true);
        else if (strcmp(argv[i], "--source-range") == 0) {
            Settings::settings()->detectStair(argv[i + 1], QString(argv[i + 2]).toInt());
            return;
        } else if (strcmp(argv[i], "--analytics") == 0) {
            analytics = true;
            if (argv[i + 1][0] != '-' && argv[i + 1][0] != 0)
                analyticsPath = QString(argv[i + 1]);
        } else if (strcmp(argv[i], "--low-memory") == 0)
            Settings::settings()->setLowMemoryMode(true);
        else if (strcmp(argv[i], "--precise-pulsar-search") == 0) {
            preciseSearch = true;
            dataPath = QString(argv[i + 1]);
        } else if (strcmp(argv[i], "--module") == 0)
            module = QString(argv[i + 1]).toInt();
        else if (strcmp(argv[i], "--ray") == 0)
            ray = QString(argv[i + 1]).toInt();
        else if (strcmp(argv[i], "--period") == 0)
            period = QString(argv[i + 1]).toDouble();
        else if (strcmp(argv[i], "--time") == 0)
            time = QTime::fromString(argv[i + 1], "hh:mm:ss");
        else if (strcmp(argv[i], "--sound-mode") == 0) {
            Settings::settings()->setSoundMode(true);\
            return;
        } else if (strcmp(argv[i], "--compress") == 0) {
            FileCompressor::compress(argv[i + 1]);
            exit(0);
        }

    if (preciseSearch) {
        qDebug() << "searching in file" << dataPath << "pulsar with period" << period << "module" << module << "ray" << ray << "with time" << time;
        Settings::settings()->setPreciseSearch(true);
        Settings::settings()->setModule(module - 1);
        Settings::settings()->setRay(ray - 1);
        Settings::settings()->setPeriod(period);
        Settings::settings()->setIntellectualFilter(false);
        Settings::settings()->setTime(time);

        if (threads != -1)
            CalculationPool::pool()->setMaxThreadCount(threads);

        PulsarProcess p(dataPath);
        p.start();
        p.wait();
//        QCoreApplication a(argc, argv);
//        a.exec();
        exit(0);
    }


    if (analytics) {
        QApplication a(argc, argv);
        a.setOrganizationDomain("bsa.vtyulb.ru");
        a.setOrganizationName("vtyulb");
        a.setApplicationName("BSA-Analytics");
        Analytics *an = new Analytics(analyticsPath);
        a.exec();
        delete an;
        exit(0);
    }

    if (dataPath == "" || savePath == "")
        exit(-1);

    savePath = QDir(savePath).absolutePath() + "/";

    QCoreApplication a(argc, argv);
    a.setOrganizationDomain("bsa.vtyulb.ru");
    a.setOrganizationName("vtyulb");
    a.setApplicationName("BSA-Analytics");

    signal(SIGSEGV, catchSigSegv);

    PulsarSearcher searcher(dataPath, savePath, threads);
    searcher.start();

    exit(a.exec());
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        pulsarEngine(argc, argv);

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
