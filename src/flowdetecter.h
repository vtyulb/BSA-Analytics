#ifndef FLOWDETECTER_H
#define FLOWDETECTER_H

#include <QDialog>
#include <QTime>
#include <QVector>

#include <data.h>

namespace Ui {
class FlowDetecter;
}

class FlowDetecter : public QDialog
{
    Q_OBJECT

public:
    explicit FlowDetecter(QWidget *parent = 0);
    ~FlowDetecter();

private:
    Ui::FlowDetecter *ui;

    int module;
    int dispersion;
    int ray;
    int points;

    double period;


    QTime time;
    QVector<double> res;
    Data data;

    QVector<double> applyDispersion();
    void subtract(double *res, int size);

private slots:
    void run();
    void setStairFileName();
    void setFileName();
};

#endif // FLOWDETECTER_H
