#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
private slots:
    void openFile();
    void readProgressChanged(double);
    void saveFile();
};

#endif // MAINWINDOW_H
