#include "nativespectredrawer.h"

#include <QPainter>
#include <QDebug>
#include <QTime>

NativeSpectreDrawer::NativeSpectreDrawer(QWidget *parent):
    QWidget(parent)
{

}

void NativeSpectreDrawer::paintEvent(QPaintEvent *event) {
    qDebug() << "drawing spectre" << QTime();
    QPainter p(this);

    p.drawImage(0, 0, spectre);

    p.end();
}
