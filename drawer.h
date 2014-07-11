#ifndef DRAWER_H
#define DRAWER_H

#include <QWidget>
#include <QFrame>
#include <data.h>
#include <nativedrawer.h>
#include <controller.h>
#include <QCheckBox>
#include <QPushButton>
#include <QCommandLinkButton>
#include <QLineEdit>

class Drawer : public QWidget
{
    Q_OBJECT
public:
    explicit Drawer(const Data data, QWidget *parent = 0);
    void saveFile(QString);
    NativeDrawer *drawer;

private:
    QFrame *controlFrame;
    QPushButton *resetButton;
    QPushButton *disableAll;
    QPushButton *enableAll;
    QCommandLinkButton *drawButton;
    QVector<QCheckBox* > checkBoxes;
    QVector<QLineEdit* > colors;
    Controller *controller;
    int rays;
signals:

private slots:
    void checkBoxStateChanged();
    void enableAllRays();
    void disableAllRays();
    void draw();
};

#endif // DRAWER_H
