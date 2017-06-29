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

    Ui::SpectreUI *ui;

private:
    QVector<double> getAnswer(const Data &data, int channel, int startPoint = -1);
    QImage drawImage(QVector<QVector<int> > matrix, const Data &data);
    QImage drawDispersion(QImage);

    Data data;
    int module, ray;
    double period;
    bool addFromMem;
    QTime time;
    QVector<QVector<double> > r;
    QVector<QVector<double> > rawRes;

    void rotateMatrix();
    int findFirstPoint(int startPoint = -1);

public slots:
    void memPlus();
    void mem();

private slots:
    void reDrawDispersion();
    void reDraw();
    void saveAs();
};

#endif // SPECTREDRAWER_H
