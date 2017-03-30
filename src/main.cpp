#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QVector>
#include <QDir>
#include <QTextBrowser>
#include <QTime>

#include <settings.h>
#include <mainwindow.h>
#include <pulsarsearcher.h>
#include <pulsarreader.h>
#include <analytics.h>
#include <calculationpool.h>
#include <filecompressor.h>
#include <spectredrawer.h>
#include <preciseperioddetecter.h>
#include <filesummator.h>
#include <flowdetecter.h>
#include <flowingwindow.h>
#include <fourier.h>
#include <fouriersearch.h>

#include <sys/unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#ifdef Q_OS_LINUX
    #include <execinfo.h>
#else
    #include <windows.h>
#endif


namespace mainSpace {
    MainWindow *w;
}

void restart(int signal = 0) {
    if (signal == SIGTERM)
        exit(0); // to avoid warnings

    qDebug() << "restarting";
    QMessageBox::warning(mainSpace::w, "Critical error", "Critical error occured!\n"
                                                         "If it's not first time,\n"
                                                         "please do write a letter\n"
                                                         "describing what have you done\n"
                                                         "to <vtyulb@vtyulb.ru>\n\n"
                                                         "Program will be restarted now");
    QProcess::startDetached(qApp->arguments().first());
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

    fprintf(stderr, "dead end, catched signal %d\n", signal);
    exit(2);
}

void precisePacket(QString me, QString fileName) {
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        while (!f.atEnd()) {
            QString line = f.readLine();
            if (line.size() < 15 || line[0] == '#')
                continue;

            QTextStream s(&line, QIODevice::ReadOnly);
            double period;
            int module, ray, D;
            QString time, name;
            s >> period >> module >> ray >> D >> time >> name;
            QStringList l;
            l << "--precise-pulsar-search" << name;
            l << "--module" << QString::number(module);
            l << "--ray" << QString::number(ray);
            l << "--period" << QString::number(period);
            l << "--dispersion" << QString::number(D);
            l << "--time" << time;
            l << "--no-multiple-periods";

            QProcess p;
            p.execute(me, l);
        }
    }
}

void makeConsoleApp(int &argc, char **argv) {
    QCoreApplication *a = new QCoreApplication(argc, argv);
    a->setOrganizationDomain("bsa.vtyulb.ru");
    a->setOrganizationName("vtyulb");
    a->setApplicationName("BSA-Analytics");
}

void makeApp(int &argc, char **argv) {
    QApplication *a = new QApplication(argc, argv);
    a->setOrganizationDomain("bsa.vtyulb.ru");
    a->setOrganizationName("vtyulb");
    a->setApplicationName("BSA-Analytics");
}

void showVersion() {
    printf("%s", Settings::settings()->version().toLocal8Bit().constData());
    exit(0);
}

void pulsarEngine(int argc, char **argv) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("BSA-Analytics usage:\n");
        printf("BSA-Analytics [311217_01_N1_00.pnt | 311217_23_N2_00.pnthr]\n");
        printf("BSA-Analytics --pulsar-search <string> --save-path <string> [--threads <int>] [--skip <int>]\n");
        printf("BSA-Analytics --analytics [path-to-data] [--fourier] [--low-memory]\n");
        printf("BSA-Analytics [--low-memory] --compress <dir>\n");
        printf("BSA-Analytics --precise-pulsar-search <file name> [--draw-spectre] --module <int> --ray <int> --period <double>\n"
               "\t[--no-multiple-periods] [--dispersion <int> ] --time <09:01:00> [--do-not-clear-noise] [--long-roads] [--normalize]\n"
               "[--period-tester] [--flux-density] [--run-analytics-after]\n");
        printf("BSA-Analytics --precise-packet <file name>\n");
        printf("BSA-Analytics --precise-timing file1 file2 file3 --module <int> --ray <int> --dispersion <int> --period <double>\n"
                "\t--time <09:01:00>\n");
        printf("BSA-Analytics --file-summator\n");
        printf("BSA-Analytics --flowing-window input-file output-file number-of-points\n");
        printf("\nOptions:\n");
        printf("\t-h --help  for this message\n");
        printf("\t-v --version for printing version information\n");
        printf("\t--pulsar-search /path/to/daily/data\n");
        printf("\t--save-path /path/to/save\n");
        printf("\t--threads <int> number of effective threads\n");
        printf("\t--skip <int> for skipping first N files\n");
        printf("\t--sub-zero for output pulsars with snr 2-5 (only good)\n");
        printf("\t--analytics to run in analytics mode\n");
        printf("\t--low-memory to do not saving data roads in analytics mode\n");
        printf("\t--long-roads to display more than two periods\n");
        printf("\t--debug for keeping debug terminal open in Windows\n");
        printf("\nWritten by Vladislav Tyulbashev.\n");
        printf("This program is distributed under GPLv3 License.\n");
        printf("Please report about any errors to <vtyulb@vtyulb.ru>\n");
        printf("or directly to bugtracker: https://github.com/vtyulb/BSA-Analytics/issues\n");
        exit(0);
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        makeConsoleApp(argc, argv);
        showVersion();
    }

    QString savePath;
    QString dataPath;
    QString analyticsPath;
    int threads = -1;
    bool analytics = false;
    bool debug = false;
    bool preciseSearch = false;
    bool drawSpectre = false;
    bool flowDetecter = false;
    bool fourier = false;
    bool runAnalyticsAfter = false;
    int module = 1;
    int ray = 1;
    double period = 1;
    QTime time(0, 0, 0);

    for (int i = 1; i < argc; i++) {
        QString cur = QString::fromLocal8Bit(argv[i]);
        QString next = QString::fromLocal8Bit(argv[i + 1]);
        if (strcmp(argv[i], "--pulsar-search") == 0)
            dataPath = next;
        else if (strcmp(argv[i], "--save-path") == 0)
            savePath = next;
        else if (strcmp(argv[i], "--threads") == 0)
            threads = QString::fromUtf8(argv[i + 1]).toInt();
        else if (strcmp(argv[i], "--skip") == 0)
            Settings::settings()->setSkipCount(next.toInt());
        else if (strcmp(argv[i], "--sub-zero") == 0)
            Settings::settings()->setSubZero(true);
        else if (strcmp(argv[i], "--analytics") == 0) {
            analytics = true;
            if (i + 1 < argc && argv[i + 1][0] != '-' && argv[i + 1][0] != 0)
                analyticsPath = next;
        } else if (strcmp(argv[i], "--low-memory") == 0)
            Settings::settings()->setLowMemoryMode(true);
        else if (strcmp(argv[i], "--precise-pulsar-search") == 0) {
            preciseSearch = true;
            dataPath = next;
        } else if (strcmp(argv[i], "--module") == 0)
            module = next.toInt();
        else if (strcmp(argv[i], "--ray") == 0)
            ray = next.toInt();
        else if (strcmp(argv[i], "--period") == 0)
            period = QString(argv[i + 1]).toDouble();
        else if (strcmp(argv[i], "--time") == 0)
            time = QTime::fromString(next, "hh:mm:ss");
        else if (strcmp(argv[i], "--compress") == 0) {
            FileCompressor::compress(next);
            exit(0);
        } else if (strcmp(argv[i], "--no-multiple-periods") == 0)
            Settings::settings()->setNoMultiplePeriods(true);
        else if (strcmp(argv[i], "--dispersion") == 0)
            Settings::settings()->setDispersion(next.toDouble());
        else if (strcmp(argv[i], "--precise-packet") == 0) {
            precisePacket(argv[0], next);
            exit(0);
        } else if (strcmp(argv[i], "--do-not-clear-noise") == 0)
            Settings::settings()->setDoNotClearNoise(true);
        else if (strcmp(argv[i], "--single-period") == 0)
            Settings::settings()->setSinglePeriod(true);
        else if (strcmp(argv[i], "--draw-spectre") == 0)
            drawSpectre = true;
        else if (strcmp(argv[i], "--long-roads") == 0)
            Settings::settings()->setLongRoads(true);
        else if (strcmp(argv[i], "--period-tester") == 0)
            Settings::settings()->setPeriodTester(true);
        else if (strcmp(argv[i], "--file-summator") == 0) {
            FileSummator summator;
            summator.run();
            exit(0);
        } else if (strcmp(argv[i], "--flowing-window") == 0) {
            FlowingWindow::run(argv[i + 1], argv[i + 2], argv[i + 3]);
            exit(0);
        } else if (strcmp(argv[i], "--fourier") == 0)
            fourier = true;
        else if (strcmp(argv[i], "--run-analytics-after") == 0)
            runAnalyticsAfter = true;
        else if (strcmp(argv[i], "--debug") == 0)
            debug = true;
        else if (strcmp(argv[i], "--flux-density") == 0)
            flowDetecter = true;
        else if (strcmp(argv[i], "--normalize") == 0)
            Settings::settings()->setNormalize(true);
    }

    if (flowDetecter) {
#ifdef WIN32
        if (!debug)
            FreeConsole();
#endif
        makeApp(argc, argv);

        FlowDetecter *detecter = new FlowDetecter(module, Settings::settings()->dispersion(),
                                                  ray, 1, false, 0, period, time, dataPath);

        detecter->run();

        exit(qApp->exec());
    }

    if (drawSpectre) {
#ifdef WIN32
        if (!debug)
            FreeConsole();
#endif
        makeApp(argc, argv);

        SpectreDrawer *sd = new SpectreDrawer;
        sd->drawSpectre(module - 1, ray - 1, dataPath, time, period);

        exit(qApp->exec());
    }

    if (QString(argv[1]) == "--precise-timing") {
        makeApp(argc, argv);

        PrecisePeriodDetecter::detect(argv[2], argv[3], argv[4], module - 1, ray - 1, Settings::settings()->dispersion(), period, time);

        exit(qApp->exec());
    }

    if (preciseSearch) {
        makeConsoleApp(argc, argv);

        qDebug() << "searching in file" << dataPath << "pulsar with period" << period << "module" << module << "ray" << ray << "with time" << time;
        Settings::settings()->setPreciseSearch(true);
        Settings::settings()->setModule(module - 1);
        Settings::settings()->setRay(ray - 1);
        Settings::settings()->setPeriod(period);
        Settings::settings()->setIntellectualFilter(false);
        Settings::settings()->setTime(time);

        if (threads != -1)
            CalculationPool::pool()->setMaxThreadCount(threads);

        QString output = QFileInfo(dataPath).fileName() + "_" + QString::number(period) + "_" + QTime::currentTime().toString("HH-mm-ss");
        output = QDir::tempPath() + "/" + output;
        qDebug() << "saving as" << output;
        if (Settings::settings()->singlePeriod())
            output += "_sp";

        if (Settings::settings()->doNotClearNoise())
            output += "_with_noise";

        PulsarProcess p(dataPath, output + "/");
        p.start();
        p.wait();

        QStringList l;
        if (runAnalyticsAfter) {
            l.clear();
            l << "--analytics" << output;
            if (Settings::settings()->lowMemory())
                l << "--low-memory";

            qDebug() << l;
            QProcess::startDetached(qApp->arguments().first(), l);
        } else {
#ifndef Q_OS_LINUX
            output.replace("/", "\\");
            l << "/select," + output;
            qDebug() << l;
            QProcess::startDetached("explorer.exe", l);
#endif
        }

        exit(0);
    }


    if (analytics) {
#ifdef WIN32
        if (!debug)
            FreeConsole();
#endif
        makeApp(argc, argv);
        Analytics *an = new Analytics(analyticsPath, fourier);
        an->show();
        QDir::setCurrent(qApp->applicationDirPath());
        exit(qApp->exec());
    }

    if (argc == 2) {
        QString data = argv[1];
        if (data.endsWith(".pnt") || data.endsWith(".pnthr")) {
            makeApp(argc, argv);

#ifdef WIN32
            FreeConsole();
#endif

            mainSpace::w = new MainWindow(data);
            mainSpace::w->show();

            exit(qApp->exec());
        }
    }

    if (argc == 2 && debug) {
        makeApp(argc, argv);
        mainSpace::w = new MainWindow;
        mainSpace::w->show();
        exit(qApp->exec());
    }

    if (dataPath == "" || savePath == "") {
        printf("You've messed up with command line options.\n");
        printf("Use 'BSA-Analytics --help' to understand why\n");
        exit(-1);
    }

    savePath = QDir(savePath).absolutePath() + "/";

    makeConsoleApp(argc, argv);

    signal(SIGSEGV, catchSigSegv);

    PulsarSearcher searcher(dataPath, savePath, threads);
    searcher.start();

    exit(qApp->exec());
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    if (argc > 1)
        pulsarEngine(argc, argv);

    signal(SIGABRT, restart);
    signal(SIGSEGV, restart);

#ifdef WIN32
    FreeConsole();
#endif

    makeApp(argc, argv);

    mainSpace::w = new MainWindow;
    mainSpace::w->show();

    try {
        qApp->exec();
    } catch (...) {
        restart();
    }

    return 0;
}
