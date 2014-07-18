#include "controller.h"
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

    layout->addWidget(rays);
    layout->addWidget(channels);
    layout->addWidget(points);
    layout->addWidget(coords);
    layout->addStretch(5);
}

void Controller::setCoords(QPoint p) {
    coords->setText(QString("X: %1; Y: %2").arg(QString::number(p.x()), QString::number(p.y())));
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
