#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <pulsar.h>
#include <pulsarlist.h>
#include <mainwindow.h>
#include <knownpulsar.h>

namespace Ui {
class Analytics;
}

class Analytics : public QWidget
{
    Q_OBJECT

public:
    explicit Analytics(QString analyticsPath = "", QWidget *parent = 0);
    ~Analytics();

    static bool goodDoubles(double, double);

private:
    Ui::Analytics *ui;
    PulsarList *list;
    MainWindow *window;
    QString folder;
    QStringList catalogs;

    Pulsars pulsars;
    QVector<bool> pulsarsEnabled;
    QVector<bool> differentNoisePreCalc;

    int maxModule;
    int maxRay;

    QVector<KnownPulsar> knownPulsars;

    void loadPulsars(QString);

    void applyPeriodFilter();
    void applySNRFilter();
    void applyModuleFilter();
    void applyRayFilter();
    void applyTimeFilter();
    void applyDuplicatesFilter();

    void applyMultiplePicksFilter();
    void applyStrangeDataFilter();
    void applyDifferentNoise();
    void applyKnownPulsarsFilter();

    void preCalc();
    void loadKnownPulsars();

private slots:
    void apply();
    void init();
    void dispersionPlot();
    void addPulsarCatalog();
    void showInfo();
    void knownPulsarsGUI();
};

#endif // ANALYTICS_H
