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
    SpectreDrawer();
    ~SpectreDrawer();
    void drawSpectre(int module, int ray, QString fileName, QTime time, double period);
    void drawSpectre(int module, int ray, const Data &_data, QTime time, double period, int startPoint = -1);

private:
    QVector<double> getAnswer(const Data &data, int channel, int startPoint = -1);
    QImage drawImage(QVector<QVector<int> > matrix, const Data &data);

    Ui::SpectreUI *ui;
    Data data;
    int module, ray;
    double period;
    bool addFromMem;
    QTime time;
    QVector<QVector<double> > r;
    QVector<QVector<double> > rawRes;

    void rotateMatrix();
    int findFirstPoint(int startPoint = -1);

private slots:
    void reDraw();
    void memPlus();
    void mem();
    void saveAs();
};

#endif // SPECTREDRAWER_H
