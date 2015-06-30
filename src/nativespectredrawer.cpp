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


    if (width() * spectre.height() / spectre.width() <= height())
        p.drawImage(QRect(0, 0, width(), width() * spectre.height() / spectre.width()), spectre);
    else
        p.drawImage(QRect(0, 0, height() * spectre.width() / spectre.height(), height()), spectre);

    p.end();
}
