#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QWidget>
#include <QColor>
#include <QPainter>
#include <QLineEdit>

class ColorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWidget(QLineEdit *l, QWidget *parent = 0);
    QLineEdit *color;
private:
    void paintEvent(QPaintEvent *);
signals:

public slots:

};

#endif // COLORWIDGET_H
