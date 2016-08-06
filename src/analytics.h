#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <pulsar.h>
#include <pulsarlist.h>
#include <mainwindow.h>
#include <knownpulsar.h>
#include <knownnoise.h>

namespace Ui {
class Analytics;
}

class Analytics : public QWidget
{
    Q_OBJECT

public:
    explicit Analytics(QString analyticsPath = "", bool fourier = false, QWidget *parent = 0);
    ~Analytics();

private:
    Ui::Analytics *ui;
    PulsarList *list;
    MainWindow *window;
    KnownNoise *noises;
    QString folder;
    QStringList catalogs;

    Pulsars pulsars;
    QVector<bool> pulsarsEnabled;
    QVector<bool> differentNoisePreCalc;
    QVector<bool> powerfullNoisePreCalc;


    int maxModule;
    int maxRay;
    bool fourier;

    QVector<KnownPulsar> knownPulsars;
    QVector<QString> fileNames;
    QVector<QVector<Data> > fourierData;

    void loadPulsars(QString);

    void applyPeriodFilter();
    void applyPeriodRangeFilter();
    void applySNRFilter();
    void applyModuleFilter();
    void applyRayFilter();
    void applyTimeFilter();
    void applyDuplicatesFilter();

    void applyMultiplePicksFilter();
    void applyStrangeDataFilter();
    void applyDifferentNoise();
    void applyKnownPulsarsFilter();
    void applyKnownNoiseFilter();
    void applyFileNameFilter();
    void applyDifferentMaximumsFilter();

    void preCalc();
    void loadKnownPulsars();

    Data dispersionGenerateData();

    void closeEvent(QCloseEvent *);

private slots:
    void apply();
    void init();
    void loadFourierData();

    void actualFourierDataChanged();

    void dispersionPlot();
    void dispersionRemember();
    void dispersionMplus();

    void addPulsarCatalog();
    void showInfo();
    void knownPulsarsGUI();
};

#endif // ANALYTICS_H
