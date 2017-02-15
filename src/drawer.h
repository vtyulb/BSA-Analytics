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
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>

class Drawer : public QWidget
{
    Q_OBJECT
public:
    explicit Drawer(const Data &data, QWidget *parent = 0);

    void saveFile(QString);
    NativeDrawer *drawer;

private:
    QFrame *controlFrame;
    QPushButton *resetButton;
    QPushButton *disableAll;
    QPushButton *enableAll;
    QDoubleSpinBox *delta;
    QCommandLinkButton *drawButton;
    QVector<QCheckBox* > checkBoxes;
    QVector<QLineEdit* > colors;
    QSpinBox *channel;
    QVector<QRadioButton* > modules;
    QVector<bool> raysEnabled;
    Controller *controller;
    int rays, numberChannels, numberModules;

    void keyPressEvent(QKeyEvent *);
signals:

private slots:
    void checkBoxStateChanged();
    void channelChanged(int);
    void moduleChanged();
    void enableAllRays();
    void disableAllRays();
    void timeToDie();
    void draw();
    void deltaChanged(double);
};

#endif // DRAWER_H
