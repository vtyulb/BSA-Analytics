#include "nativedrawer.h"
#include <QRgb>
#include <QApplication>

NativeDrawer::NativeDrawer(const Data data, QWidget *parent) :
    QWidget(parent),
    art(NULL),
    data(data),
    mousePressed(false),
    allowDrawing(false),
    autoDrawing(true),
    drawAxesFlag(true),
    drawNet(false)
{
    for (int i = 0; i < data[0].size(); i++)
        rayVisibles.push_back(true);

    resetVisibleRectangle();
    setMouseTracking(true);
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

    delete art;
    qDebug() << screen.bottomLeft() << screen.topRight();
    if (width() < 100 || height() < 100) {
        drawing.unlock();
        return;
    }

    art = new QImage(this->width(), this->height(), QImage::Format_RGB32);

    QPainter p(art);
    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("white"));
    p.drawRect(0, 0, width(), height());
    p.setPen(QPen("black"));
    qDebug() << width() << height();

    int rays = data[0].size();
    for (int k = 0; k < data.size() / 50000 + 1; k++) {
        repaint();
//        qApp->processEvents();
        emit progress(5000000*k/data.size());
        for (int j = 0; j < rays; j++) {
            QByteArray c = QByteArray::fromHex(colors[j].toUtf8());
            p.setPen(QColor((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));

            if (rayVisibles[j])
                for (int i = k * 50000 + 1; i < minimum(data.size(), (k + 1)*50000 + 1); i++) {
                    int x = newCoord(i, 0).x();
                    if (x < 0 || x > art->width())
                        continue;

                    p.drawLine(mirr(newCoord(i, data[i][j])), mirr(newCoord(i - 1, data[i - 1][j])));
                }
        }
    }

    emit progress(100);
    p.end();
    drawAxes();

    drawing.unlock();
    repaint();
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
    if (mousePressed)
        repaint();

    emit coordsChanged(backwardCoord(mirr(event->pos())));
}

void NativeDrawer::mouseReleaseEvent(QMouseEvent *event) {
    mousePressed = false;

    if (abs(mouseRect.width()) < 10 || abs(mouseRect.height()) < 10)
        return;

    if (mouseClicked.x() > event->pos().x() || mouseClicked.y() > event->pos().y()) {
        int h = abs(screen.height()) / 2;
        int w = screen.width() / 2;
        screen.setRight(screen.right() + w);
        screen.setLeft(screen.left() - w);
        screen.setBottom(screen.bottom() - h);
        screen.setTop(screen.top() + h);
        qDebug() << screen;
    } else {
        QRect c;
        //WTF How it works?!
        int top = mouseRect.top() + 10;
        int bot = mouseRect.bottom() - 10;

        mouseRect.setTop(height() - bot);
        mouseRect.setBottom(height() - top);
        mouseRect.setLeft(mouseRect.left() - 10);
        mouseRect.setRight(mouseRect.right() + 10);

        c.setTopLeft(backwardCoord(mouseRect.bottomLeft()));
        c.setBottomRight(backwardCoord(mouseRect.topRight()));
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
