#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <reader.h>
#include <drawer.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static QString nativeDecodeLastPath(QString path);

private:
    Ui::MainWindow *ui;
    Drawer *drawer;
    QProgressBar *progress;

    QString lastOpenPath;

    void saveSettings();
    void loadSettings();

    void decodeLastPath(QString path);

private slots:
    void openPulsarFile();
    void openAnalytics(bool hasMemory = true);
    void openFile();
    void openBinaryFile();
    void customOpen();

    void nativeOpenFile(QString fileName, int skip = 0, int skipFirstRay = 2, QDateTime = QDateTime(), bool binary = false);
    void readProgressChanged(double);
    void saveFile();
    void autoDraw(bool);
    void drawAxes(bool);
    void drawNet(bool);
    void drawFast(bool);
    void drawLive(bool);
    void showAbout();
    void showAboutQt();

    void runPreciseGui();
    void runPreciseTimingGui();
    void runFlowGui();
    void runRotationGui();

    void clone();
    void soundModeTriggered();

public slots:
    void regenerate(Data &data);

};

#endif // MAINWINDOW_H
