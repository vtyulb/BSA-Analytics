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

private:
    QLabel *coords;
    QLabel *rays;
    QLabel *points;
    QLabel *channels;
    QLabel *modules;
    QLabel *sky;

    Data data;

signals:

public slots:
    void setCoords(QPointF);
    void resetSky(Data newData);
};

#endif // CONTROLLER_H
