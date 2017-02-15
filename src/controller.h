#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QFrame>
#include <QLabel>
#include <QPoint>
#include <data.h>

class Controller : public QFrame
{
    Q_OBJECT
public:
    explicit Controller(QWidget *parent = 0);

    void setRays(int);
    void setPoints(int);
    void setChannels(int);
    void setModules(int);
    void setFileName(QString);

private:
    QLabel *coords;
    QLabel *rays;
    QLabel *points;
    QLabel *channels;
    QLabel *modules;
    QLabel *sky;
    QLabel *fileName;
    QLabel *nativeXCoord;

    Data data;

signals:

public slots:
    void setCoords(QPointF);
    void resetSky(Data newData, int module = 1, QVector<bool> ray = QVector<bool>());
};

#endif // CONTROLLER_H
