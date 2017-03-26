#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QLayout>

#include <reader.h>
#include <drawer.h>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString file = "", QWidget *parent = 0);
    ~MainWindow();

    static QString nativeDecodeLastPath(QString path);
    void addWidgetToMainLayout(QWidget *w1, QWidget *w2);

private:
    Ui::MainWindow *ui;
    Drawer *drawer;
    QProgressBar *progress;
    QMenu *styles;

    QString lastOpenPath;
    QString fileToOpen;

    void saveSettings();
    void loadSettings();
    void generateStyles();

    void decodeLastPath(QString path);

private slots:
    void openPulsarFile();
    void openAnalytics(bool hasMemory = true, bool fourier = false);
    void openFourierAnalytics();
    void openFile();
    void openBinaryFile();
    void openStartFile();
    void customOpen();

    void nativeOpenFile(QString fileName, int skip = 0, int skipFirstRay = 2, QDateTime = QDateTime(), bool binary = false);
    void readProgressChanged(double);
    void saveFile();
    void autoDraw(bool);
    void drawAxes(bool);
    void drawNet(bool);
    void drawFast(bool);
    void drawLive(bool);

    void showHelp();
    void showAbout();
    void showAboutQt();

    void runPreciseGui();
    void runPreciseTimingGui();

    void clone();
    void soundModeTriggered();

    void setRotationMeasureMode();
    void setFluxDensityMode();
    void normalizeData();

    void setStyle(QAction*);
    void checkForUpdatesStatusChanged();

public slots:
    void regenerate(const Data &data);
};

#endif // MAINWINDOW_H
