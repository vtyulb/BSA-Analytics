#ifndef NATIVEDRAWER_H
#define NATIVEDRAWER_H

#include <QWidget>
#include <data.h>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QRect>
#include <QImage>
#include <QDebug>
#include <QMutex>
#include <QStack>
#include <QtPrintSupport/QPrinter>

class NativeDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit NativeDrawer(const Data &data, QWidget *parent = 0);
    ~NativeDrawer();

    void setData(const Data &newData);
    void setRayVisibles(QVector<bool>);
    void setColors(QVector<QString>);
    void saveFile(QString);
    void nativePaint(bool forPrinter = false);
    void drawAxes();

    bool allowDrawing;
    bool autoDrawing;
    bool drawAxesFlag;
    bool drawNet;
    bool drawFast;
    bool live;

    QMutex drawing;

    int channel;
    int module;

    Data data;

private:
    QVector<bool> rayVisibles;
    QVector<QString> colors;
    QStack<QRectF> screens;
    QRectF screen;
    QImage *art;

    void paintEvent(QPaintEvent *);

    QPoint newCoord(float x, float y);
    QPointF backwardCoord(QPointF);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void leaveEvent(QEvent *event);

    bool mousePressed;
    QPoint mouseClicked;
    QRect mouseRect;

    int verticalLine;

    int minimum(int, int);

    void sourceDetect(int, int);
    void fourierDraw(QPainter &p);

    QPoint mirr(QPoint);

signals:
    void progress(int);
    void coordsChanged(QPointF);

public slots:
    void resetVisibleRectangle(bool repaint = true, bool resetLeftRight = true);
    void print();

private slots:
    void nativePrint(QPrinter *);

};

#endif // NATIVEDRAWER_H
