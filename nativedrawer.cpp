#include "nativedrawer.h"
#include <QRgb>

NativeDrawer::NativeDrawer(const Data data, QWidget *parent) :
    QWidget(parent),
    art(NULL),
    data(data),
    mousePressed(false),
    allowDrawing(false)
{
    for (int i = 0; i < data[0].size(); i++)
        rayVisibles.push_back(true);

    resetVisibleRectangle();
}

void NativeDrawer::setRayVisibles(QVector<bool> v) {
    rayVisibles = v;
    nativePaint();
}

void NativeDrawer::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    if (art)
        p.drawImage(QRect(0, 0, width(), height()), *art);

    p.setPen(QColor("black"));
    p.setBrush(QBrush(QColor(0, 50, 200, 100)));
    if (mousePressed)
        p.drawRect(mouseRect);

    p.end();
    event->accept();
}

void NativeDrawer::nativePaint() {
    if (!allowDrawing)
        return;

    delete art;
    qDebug() << screen.bottomLeft() << screen.topRight();
    if (width() < 100 || height() < 100)
        return;

    art = new QImage(this->width(), this->height(), QImage::Format_RGB32);

    QPainter p(art);
    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("white"));
    p.drawRect(0, 0, width(), height());
    p.setPen(QPen("black"));
    qDebug() << width() << height();

    int rays = data[0].size();
    for (int j = 0; j < rays; j++) {
        QByteArray c = QByteArray::fromHex(colors[j].toUtf8());
        p.setPen(QColor((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));

        if (rayVisibles[j])
            for (int i = 1; i < data.size(); i++)
                p.drawLine(newCoord(i, data[i][j]), newCoord(i - 1, data[i - 1][j]));
    }

    p.end();
    reflect();
    repaint();
}

void NativeDrawer::reflect() {
    for (int i = 0; i < width(); i++)
        for (int j = 0; j < height() / 2; j++) {
            QRgb c1 = art->pixel(i, j);
            QRgb c2 = art->pixel(i, art->height() - j - 1);
            art->setPixel(i, j, c2);
            art->setPixel(i, height() - 1 - j, c1);
        }
}

void NativeDrawer::resetVisibleRectangle() {
    int min = 1000 * 1000 * 1000;
    int max = -min;

    for (int i = 0; i < data.size(); i++)
        for (int j = 0; j < data[i].size(); j++) {
            if (data[i][j] > max)
                max = data[i][j];

            if (data[i][j] < min)
                min = data[i][j];
        }

    screen.setBottomLeft(QPoint(0, min));
    screen.setTopRight(QPoint(data.size(), max));
    nativePaint();
}

QPoint NativeDrawer::newCoord(int x, int y) {
    QPoint res;
    res.setX((x - screen.left())*art->width()/(screen.right()-screen.left()));
    res.setY((y - screen.bottom())*art->height()/(screen.top()-screen.bottom()));
    return res;
}

QPoint NativeDrawer::backwardCoord(QPoint p) {
    QPoint res;
    res.setX(screen.left() + (screen.right()-screen.left())*p.x()/art->width());
    res.setY(screen.bottom() + (screen.top()-screen.bottom())*p.y()/art->height());
    return res;
}

void NativeDrawer::mousePressEvent(QMouseEvent *event) {
    mousePressed = true;
    mouseClicked = event->pos();
}

void NativeDrawer::mouseMoveEvent(QMouseEvent *event) {
    mouseRect = QRect(mouseClicked, event->pos());
    repaint();
}

void NativeDrawer::mouseReleaseEvent(QMouseEvent *event) {
    mousePressed = false;

    if (mouseRect.width() < 10 || mouseRect.height() < 10)
        return;

    if (mouseClicked.x() > event->pos().x()) {
        QRect c;
        c.setBottomLeft(backwardCoord(QPoint(-width(), -height())));
        c.setTopRight(backwardCoord(QPoint(width() * 2, height() * 2)));
        screen = c;
    } else {
        QRect c;
        //WTF How it works?!
        int top = mouseRect.top();
        int bot = mouseRect.bottom();

        mouseRect.setTop(height() - bot);
        mouseRect.setBottom(height() - top);

        c.setTopLeft(backwardCoord(mouseRect.bottomLeft()));
        c.setBottomRight(backwardCoord(mouseRect.topRight()));
        screen = c;
    }

    nativePaint();
    repaint();
}

void NativeDrawer::resizeEvent(QResizeEvent *event) {
    nativePaint();
}

void NativeDrawer::saveFile(QString file) {
    art->save(file, file.right(3).toUtf8().constData());
}

void NativeDrawer::setColors(QVector<QString> c) {
    colors = c;
}
