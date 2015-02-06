#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <pulsar.h>
#include <pulsarlist.h>
#include <mainwindow.h>

namespace Ui {
class Analytics;
}

class Analytics : public QWidget
{
    Q_OBJECT

public:
    explicit Analytics(QWidget *parent = 0);
    ~Analytics();

private:
    Ui::Analytics *ui;
    PulsarList *list;
    MainWindow *window;

    Pulsars pulsars;
    QVector<bool> pulsarsEnabled;

    void loadPulsars(QString);

    void applyPeriodFilter();
    void applySNRFilter();
    void applyModuleFilter();
    void applyRayFilter();
    void applyTimeFilter();

private slots:
    void apply();
};

#endif // ANALYTICS_H