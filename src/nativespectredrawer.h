#ifndef NATIVESPECTREDRAWER_H
#define NATIVESPECTREDRAWER_H

#include <QWidget>
#include <QImage>

class NativeSpectreDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit NativeSpectreDrawer(QWidget *parent = 0);
    QImage spectre;

private:
    void paintEvent(QPaintEvent*);

signals:

public slots:
};

#endif // NATIVESPECTREDRAWER_H
