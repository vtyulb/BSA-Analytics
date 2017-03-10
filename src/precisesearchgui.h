#ifndef PRECISESEARCHGUI_H
#define PRECISESEARCHGUI_H

#include <QDialog>

namespace Ui {
    class PreciseSearchGui;
}

class PreciseSearchGui : public QDialog
{
    Q_OBJECT

public:
    explicit PreciseSearchGui(QWidget *parent = 0);
    ~PreciseSearchGui();

private slots:
    void selectFile();
    void runSearcher();
    void runPacketSearcher();

    void determineSearchMode();

private:
    Ui::PreciseSearchGui *ui;
};

#endif // PRECISESEARCHGUI_H
