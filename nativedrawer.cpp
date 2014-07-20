#include "nativedrawer.h"
#include <QRgb>
#include <QApplication>

NativeDrawer::NativeDrawer(const Data &data, QWidget *parent) :
    QWidget(parent),
    allowDrawing(false),
    autoDrawing(true),
    drawAxesFlag(true),
    drawNet(false),
    channel(0),
    module(0),
    art(new QImage(100, 100, QImage::Format_ARGB32)),
    data(data),
    mousePressed(false)
{
    for (int i = 0; i < data.rays; i++)
        rayVisibles.push_back(true);

    resetVisibleRectangle();
    setMouseTracking(true);
}

NativeDrawer::~NativeDrawer() {
    delete art;
    data.releaseData();
    qDebug() << "data released";
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

    if (!drawing.tryLock())
        return;

    qDebug() << screen.bottomLeft() << screen.topRight();
    if (width() < 100 || height() < 100) {
        drawing.unlock();
        return;
    }

    delete art;
    art = new QImage(this->width(), this->height(), QImage::Format_RGB32);

    QPainter p(art);
    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("white"));
    p.drawRect(0, 0, width(), height());
    p.setPen(QPen("black"));
    qDebug() << width() << height();

    int rays = data.rays;
    for (int k = 0; k < data.npoints / 50000 + 1; k++) {
        if (k)
            repaint();

        emit progress(5000000*k/data.npoints);


        for (int j = 0; j < rays; j++) {
            QByteArray c = QByteArray::fromHex(colors[j].toUtf8());
            p.setPen(QColor((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));

            if (rayVisibles[j])
                for (int i = k * 50000 + 1 + drawFast * 5; i < minimum(data.npoints, (k + 1)*50000 + 1); i += 1 + drawFast * 5) {
                    int x = newCoord(i, 0).x();
                    if (x < 0 || x > art->width())
                        continue;

                    p.drawLine(mirr(newCoord(i, data.data[module][channel][j][i])), mirr(newCoord(i - 1 - drawFast * 5, data.data[module][channel][j][i - 1])));
                }
        }
    }

    emit progress(100);
    p.end();
    drawAxes();

    drawing.unlock();
    repaint();
}

void NativeDrawer::resetVisibleRectangle(bool repaint) {
    float min = 1e+30;
    float max = -min;

    for (int i = 0; i < data.npoints; i++)
        for (int j = 0; j < data.rays; j++) {
            if (data.data[module][channel][j][i] > max)
                max = data.data[module][channel][j][i];

            if (data.data[module][channel][j][i] < min)
                min = data.data[module][channel][j][i];
        }

    float deltaX = data.npoints * 0.05;
    float deltaY = (max - min) * 0.05;

    screen.setBottomLeft(QPointF(0 - deltaX, min - deltaY));
    screen.setTopRight(QPointF(data.npoints + deltaX, max + deltaY));
    if (repaint)
        nativePaint();
}

QPoint NativeDrawer::newCoord(float x, float y) {
    QPoint res;
    res.setX((x - screen.left())*art->width()/(screen.right()-screen.left()));
    res.setY((y - screen.bottom())*art->height()/(screen.top()-screen.bottom()));
    return res;
}

QPointF NativeDrawer::backwardCoord(QPointF p) {
    QPointF res;
    res.setX(screen.left() + (screen.right()-screen.left())*p.x()/art->width());
    res.setY(screen.bottom() + (screen.top()-screen.bottom())*p.y()/art->height());
    return res;
}

void NativeDrawer::mousePressEvent(QMouseEvent *event) {
    if (!drawing.tryLock())
        return;

    mousePressed = true;
    mouseClicked = event->pos();
    drawing.unlock();
}

void NativeDrawer::mouseMoveEvent(QMouseEvent *event) {
    mouseRect = QRect(mouseClicked, event->pos());
    if (mousePressed)
        repaint();

    emit coordsChanged(backwardCoord(mirr(event->pos())));
}

void NativeDrawer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mousePressed)
        return;

    mousePressed = false;

    if (abs(mouseRect.width()) < 50 || abs(mouseRect.height()) < 50) {
        repaint();
        return;
    }

    if (mouseClicked.x() > event->pos().x() || mouseClicked.y() > event->pos().y()) {
        if (screens.size())
            screen = screens.pop();
        else {
            repaint();
            return;
        }
    } else {
        QRectF c;
        //WTF How it works?!
        int top = mouseRect.top();
        int bot = mouseRect.bottom();

        mouseRect.setTop(height() - bot);
        mouseRect.setBottom(height() - top);

        c.setTopLeft(backwardCoord(mouseRect.bottomLeft()));
        c.setBottomRight(backwardCoord(mouseRect.topRight()));

        float deltaY = (c.top() - c.bottom()) * 0.05;
        float deltaX = (c.right() - c.left()) * 0.05;

        c.setTop(c.top() + deltaY);
        c.setBottom(c.bottom() - deltaY);
        c.setLeft(c.left() - deltaX);
        c.setRight(c.right() + deltaX);

        screens.push_back(screen);
        screen = c;
    }

    nativePaint();
    repaint();
}

void NativeDrawer::resizeEvent(QResizeEvent *event) {
    if (autoDrawing)
        nativePaint();

    event->accept();
}

void NativeDrawer::saveFile(QString file) {
    art->save(file, file.right(3).toUtf8().constData());
}

void NativeDrawer::setColors(QVector<QString> c) {
    colors = c;
}

void NativeDrawer::drawAxes() {
    if (!drawAxesFlag)
        return;

    QPainter p(art);

    p.setPen(QPen(QColor("white")));
    p.setBrush(QBrush(QColor("white")));
    p.drawRect(0, 0, 46, art->height());
    p.drawRect(0, height() - 25, art->width(), height());

    p.setPen(QColor("black"));

    p.drawLine(QPoint(0, art->height() - 3), QPoint(art->width(), art->height() - 3));
    for (int i = 1; i <= 25; i++) {
        p.drawLine(QPoint(art->width() / 26 * i, art->height()), QPoint(art->width() / 26 * i, art->height() - 6 - (4 + art->height() * drawNet) * ((i-1)%5 == 0)));
        if ((i - 1)%5==0)
            p.drawText(QPoint(art->width() / 26 * i + 1, art->height() - 12), QString::number(backwardCoord(QPoint(art->width()/26 * i, 0)).x()));
    }

    p.drawLine(QPoint(3, 0), QPoint(3, art->height()));
    for (int i = 1; i <= 25; i++) {
        p.drawLine(QPoint(0, art->height() / 26 * i), QPoint(6 + (4 + art->width() * drawNet) * (i%5==0), art->height() / 26 * i));
        if (i%5==0)
            p.drawText(QPoint(5, art->height() / 26 * i + 12), QString::number(backwardCoord(mirr(QPoint(0,art->height()/26*i))).y()));
    }
}

int NativeDrawer::minimum(int a, int b) {
    if (a < b)
        return a;
    else
        return b;
}

QPoint NativeDrawer::mirr(QPoint p) {
    p.setY(art->height() - p.y());
    return p;
}

void NativeDrawer::leaveEvent(QEvent *event) {
    qDebug() << "leave event";
    mousePressed = false;
    repaint();
    event->accept();
}
