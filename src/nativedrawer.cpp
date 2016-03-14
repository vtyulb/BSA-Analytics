#include "nativedrawer.h"
#include <QRgb>
#include <QApplication>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QMessageBox>

#include <startime.h>
#include <cmath>
#include <settings.h>
#include <wavplayer.h>

NativeDrawer::NativeDrawer(const Data &data, QWidget *parent) :
    QWidget(parent),
    allowDrawing(false),
    autoDrawing(true),
    drawAxesFlag(true),
    drawNet(false),
    channel(0),
    module(0),
    data(data),
    art(new QImage(100, 100, QImage::Format_ARGB32)),
    mousePressed(false),
    verticalLine(-1)
{
    for (int i = 0; i < data.rays; i++)
        rayVisibles.push_back(true);

    resetVisibleRectangle();
    setMouseTracking(true);
    Settings::settings()->setLastData(data);
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
    if (mousePressed) {
        if (Settings::settings()->sourceMode()) {
            mouseRect.setBottom(0);
            mouseRect.setTop(height());
        }
        p.drawRect(mouseRect);
    } else if (Settings::settings()->sourceMode())
        p.drawLine(verticalLine, 0, verticalLine, height());

    p.end();
    event->accept();
}

void NativeDrawer::nativePaint(bool forPrinter) {
    if (!allowDrawing)
        return;

    if (!drawing.tryLock())
        return;

    if (width() < 100 || height() < 100) {
        drawing.unlock();
        return;
    }

    if (!forPrinter) {
        delete art;
        art = new QImage(this->width(), this->height(), QImage::Format_RGB32);
    }

    QPainter p(art);
    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("white"));
    p.drawRect(0, 0, art->width(), art->height());
    p.setPen(QPen("black"));

    int rays = data.rays;
    for (int k = 0; k < data.npoints / 50000 + 1; k++) {
        if (k)
            if (live)
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

    if (data.sigma > 0) {
//        p.setPen(QColor("blue"));
//        p.drawLine(mirr(newCoord(0, data.sigma)), mirr(newCoord(width(), data.sigma)));

//        p.setPen(QColor("green"));
//        p.drawLine(mirr(newCoord(0, 0)), mirr(newCoord(width(), 0)));
        p.setPen(QColor("green"));
//        for (double i = data.oneStep; i < data.npoints; i += )

    }

    emit progress(100);
    p.end();
    drawAxes();

    drawing.unlock();
    repaint();
}

void NativeDrawer::resetVisibleRectangle(bool repaint, bool resetLeftRight) {

    screens.clear();

    float min = 1e+30;
    float max = -min;

    for (int i = 0; i < data.npoints; i++)
         for (int j = 0; j < data.rays; j++)
//             for (int channel = 0; channel < data.channels; channel++)
                 if (!std::isinf(data.data[module][channel][j][i])) {
                     if (data.data[module][channel][j][i] > max)
                         max = data.data[module][channel][j][i];

                     if (data.data[module][channel][j][i] < min)
                         min = data.data[module][channel][j][i];
                 } else {
                     qDebug() << "error at point" << i << "at ray" << j;
                 }

    float deltaX = data.npoints * 0.08;
    float deltaY = (max - min) * 0.05;

    if (resetLeftRight) {
        screen.setBottomLeft(QPointF(0 - deltaX, min - deltaY));
        screen.setTopRight(QPointF(data.npoints + deltaX, max + deltaY));
    } else {
        screen.setTop(max + deltaY);
        screen.setBottom(min - deltaY);
    }

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
    mouseRect = QRect(mouseClicked, mouseClicked);
    drawing.unlock();
}

void NativeDrawer::mouseMoveEvent(QMouseEvent *event) {
    verticalLine = event->pos().x();
    mouseRect = QRect(mouseClicked, event->pos());
    if (mousePressed || Settings::settings()->sourceMode())
        repaint();

    emit coordsChanged(backwardCoord(mirr(event->pos())));
}

void NativeDrawer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mousePressed)
        return;

    mousePressed = false;

    if (Settings::settings()->sourceMode() && mouseRect.width() > 0) {
        sourceDetect(backwardCoord(mouseRect.topLeft()).x(), backwardCoord(mouseRect.bottomRight()).x());
        return;
    }

    if (abs(mouseRect.width()) < 50 || abs(mouseRect.height()) < 50) {
        repaint();
        return;
    }

    if (Settings::settings()->soundMode()) { //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        qDebug() << "playing";
        int x1 = backwardCoord(mouseRect.topLeft()).x();
        int x2 = backwardCoord(mouseRect.bottomRight()).x();
        QVector<double> sound;
        int ray = 0;
        for (; ray < data.rays; ray++)
            if (rayVisibles[ray])
                break;

        for (int i = x1; i < x2; i++)
            sound.push_back(data.data[module][channel][ray][i]);

        WavPlayer::play(sound, data.oneStep * 1000 + 1);
        return;
    }

    if (mouseClicked.x() > event->pos().x() || mouseClicked.y() > event->pos().y()) {
        if (screens.size())
            screen = screens.pop();
        else {
            resetVisibleRectangle();
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
    QByteArray ext = file.right(file.size() - file.indexOf('.') - 1).toUtf8();
    if (ext.size() > 6) {
        file += ".png";
        ext = "png";
    }

    art->save(file, ext.constData());
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
    p.drawRect(0, 0, art->width(), 25);
    p.drawRect(0, height() - 34, art->width(), height());
    p.drawRect(art->width() - 25, 0, art->width(), art->height());

    p.setPen(QColor("black"));

    p.drawLine(QPoint(0, art->height() - 3), QPoint(art->width(), art->height() - 3));
    p.drawLine(QPoint(0, 3), QPoint(art->width(), 3));
    for (int i = 50; i < art->width() - 20; i+=50) {
        if (drawNet) {
            QPen backup = p.pen();
            p.setPen(Qt::DotLine);
            p.drawLine(QPoint(i, 0), QPoint(i, height()));
            p.setPen(backup);
        }

        p.drawLine(QPoint(i, art->height()), QPoint(i, art->height() - 6 - (4 + art->height() * drawNet) * (i%250 == 0)));
        p.drawLine(QPoint(i, 0), QPoint(i, 6 + 4 * (i%250 == 0)));
        if (i%250==0) {
            p.drawText(QPoint(i + 1, art->height() - 12), QString::number(backwardCoord(QPoint(i, 0)).x()));
            p.drawText(QPoint(i + 1, art->height() - 24), StarTime::StarTime(data, backwardCoord(QPointF(i, 0)).x()));
        }
    }

    p.drawLine(QPoint(3, 0), QPoint(3, art->height()));
    p.drawLine(QPoint(art->width() - 3, 0), QPoint(art->width() - 3, art->height()));
    for (int i = 50; i < art->height() - 20; i+=50) {
        if (drawNet) {
            QPen backup = p.pen();
            p.setPen(Qt::DotLine);
            p.drawLine(QPoint(0, i), QPoint(width(), i));
            p.setPen(backup);
        }

        p.drawLine(QPoint(0, i), QPoint(6 + (4 + art->width() * drawNet) * (i%250==0), i));
        p.drawLine(QPoint(art->width() - 6 - 4 * (i%250==0), i), QPoint(art->width(), i));
        if (i%250==0)
            p.drawText(QPoint(5, i + 12), QString::number(backwardCoord(mirr(QPoint(0,i))).y()));
    }

    p.end();
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
    mousePressed = false;
    repaint();
    event->accept();
}

void NativeDrawer::print() {
    QPrintDialog *dialog = new QPrintDialog(this);
    QObject::connect(dialog, SIGNAL(accepted(QPrinter*)), this, SLOT(nativePrint(QPrinter*)));
    dialog->exec();
}

void NativeDrawer::nativePrint(QPrinter *printer) {
    printer->setOrientation(QPrinter::Landscape);
//    print 5er->setColorMode(QPrinter::Color);
    bool tmp = live;
    live = false;
    QImage *tmpArt = art;
    art = new QImage(printer->width(), printer->height(), QImage::Format_ARGB32);
    nativePaint(true);
    live = tmp;

    QPainter painter(printer);
    painter.drawImage(QRect(0, 0, art->width() - 1, art->height() - 1), *art);
    painter.end();

    delete art;
    art = tmpArt;

    repaint();
}

void NativeDrawer::sourceDetect(int a, int b) {
    // What is it doing here?

    int ray = 0;
    for (int i = 0; i < data.rays; i++)
        if (rayVisibles[i]) {
            ray = i;
            break;
        }

    QString resStr = "Ray: " + QString::number(ray + 1) + "\n";
    for (int k = 0; k < data.channels - 1; k++) {
        QVector<double> res;
        double h1 = data.data[module][k][ray][a];
        double h2 = data.data[module][k][ray][b];
        for (int i = a; i < b; i++)
            res.push_back(data.data[module][k][ray][i] - ((i - a) / double(b - a) * (h2 - h1) + h1));

        std::sort(res.data(), res.data() + res.size());
        resStr = resStr + " " + QString::number(res[res.size() - 3] / Settings::settings()->getStairHeight(module, ray, k));
    }

    QMessageBox::information(this, "source height", resStr);
}
