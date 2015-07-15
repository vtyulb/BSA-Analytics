#ifndef SPECTREDRAWER_H
#define SPECTREDRAWER_H

#include <QString>
#include <QTime>
#include <QVector>

#include <data.h>
#include "ui_spectre.h"
#include <nativespectredrawer.h>

class SpectreDrawer: public QWidget
{
    Q_OBJECT

public:
    SpectreDrawer() {};
    void drawSpectre(int module, int ray, QString fileName, QTime time, double period);

private:
    QVector<double> getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period);
    QImage drawImage(QVector<QVector<int> > matrix, const Data &data);

    Ui::SpectreUI *ui;
    Data data;
    int module, ray;
    double period;
    QTime time;
    QVector<QVector<double> > r;

public slots:
    void reDraw();
    void saveAs();
};

#endif // SPECTREDRAWER_H
