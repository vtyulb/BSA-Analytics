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

private:
    Ui::MainWindow *ui;
    Drawer *drawer;
    QProgressBar *progress;

    void saveSettings();
    void loadSettings();

private slots:
    void openFile();
    void readProgressChanged(double);
    void saveFile();
    void autoDraw(bool);
    void drawAxes(bool);
    void drawNet(bool);
    void showAbout();
    void showAboutQt();
};

#endif // MAINWINDOW_H
