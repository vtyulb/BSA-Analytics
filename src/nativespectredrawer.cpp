#include "nativespectredrawer.h"

#include <QPainter>
#include <QDebug>
#include <QTime>

NativeSpectreDrawer::NativeSpectreDrawer(QWidget *parent):
    QWidget(parent)
{
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void NativeSpectreDrawer::paintEvent(QPaintEvent*) {
    QPainter p(this);

    if (spectre.isNull())
        return;

    if (width() * spectre.height() / spectre.width() <= height())
        p.drawImage(QRect(0, 0, width(), width() * spectre.height() / spectre.width()), spectre);
    else
        p.drawImage(QRect(0, 0, height() * spectre.width() / spectre.height(), height()), spectre);

    p.end();
}
