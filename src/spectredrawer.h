#ifndef SPECTREDRAWER_H
#define SPECTREDRAWER_H

#include <QString>
#include <QTime>
#include <QVector>

#include <data.h>

class SpectreDrawer
{
public:
    static void drawSpectre(int module, int ray, QString fileName, QTime time, double period);

private:
    static QVector<int> getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period);
    static void drawImage(QVector<QVector<int> > matrix);

    SpectreDrawer();
};

#endif // SPECTREDRAWER_H
