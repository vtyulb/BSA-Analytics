#ifndef NATIVEDRAWER_H
#define NATIVEDRAWER_H

#include <QWidget>
#include <data.h>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QImage>
#include <QDebug>
#include <QMutex>

class NativeDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit NativeDrawer(const Data data, QWidget *parent = 0);

    void setRayVisibles(QVector<bool>);
    void setColors(QVector<QString>);
    void saveFile(QString);
    void nativePaint();

    bool allowDrawing;
    bool autoDrawing;
    QMutex drawing;

private:
    QVector<bool> rayVisibles;
    QVector<QString> colors;
    QRect screen;
    QImage *art;
    const Data data;

    void paintEvent(QPaintEvent *);

    QPoint newCoord(int x, int y);
    QPoint backwardCoord(QPoint);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

    bool mousePressed;
    QPoint mouseClicked;
    QRect mouseRect;

    void reflect();

signals:

public slots:
    void resetVisibleRectangle();

};

#endif // NATIVEDRAWER_H
