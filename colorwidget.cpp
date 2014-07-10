#include "colorwidget.h"
#include <QTimer>

ColorWidget::ColorWidget(QLineEdit *l, QWidget *parent) :
    QWidget(parent),
    color(l)
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(3000);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start();
}

void ColorWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    QByteArray c = QByteArray::fromHex(color->text().toUtf8());
    p.setBrush(QColor((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));
    p.drawRect(0, 0, width(), height());
    p.end();
}
