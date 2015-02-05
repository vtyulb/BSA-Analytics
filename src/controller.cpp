#include "controller.h"
#include <startime.h>
#include <QVBoxLayout>

Controller::Controller(QWidget *parent) :
    QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    QVBoxLayout *layout = new QVBoxLayout(this);

    coords = new QLabel(this);
    rays = new QLabel(this);
    points = new QLabel(this);
    channels = new QLabel(this);
    modules = new QLabel(this);
    sky = new QLabel(this);
    fileName = new QLabel(this);
    nativeXCoord = new QLabel(this);

    layout->addWidget(rays);
    layout->addWidget(channels);
    layout->addWidget(modules);
    layout->addWidget(points);
    layout->addWidget(sky);
    layout->addWidget(coords);
    layout->addWidget(nativeXCoord);
    layout->addWidget(fileName);
    layout->addStretch(5);
}

void Controller::setCoords(QPointF p) {
    coords->setText(QString("X: %1; Y: %2").arg(StarTime::StarTime(data, p.x()), QString::number(p.y())));
    nativeXCoord->setText(QString("X: %1").arg(p.x()));
}

void Controller::setRays(int r) {
    rays->setText(QString("%1 rays detected").arg(QString::number(r)));
}

void Controller::setPoints(int p) {
    points->setText(QString("%1 points on ray").arg(QString::number(p)));
}

void Controller::setChannels(int c) {
    channels->setText(QString("%1 channels").arg(QString::number(c)));
}

void Controller::setModules(int m) {
    modules->setText(QString("%1 modules").arg(QString::number(m)));
}

void Controller::resetSky(Data newData) {
    data = newData;
    sky->setText(StarTime::StarTime(data));
}

void Controller::setFileName(QString s) {
    fileName->setText(s);
}
