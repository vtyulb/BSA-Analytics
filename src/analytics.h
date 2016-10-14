#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <QSet>

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
    int fourierSpectreSize;
    bool longData;

    QVector<KnownPulsar> knownPulsars;
    QVector<QString> fileNames;
    QVector<Data> fourierData;
    QVector<float> fourierSumm[6][8];
    QVector<double> fourierRawNoises[6][8];
    QSet<QString> fourierAllowedNames;

    void parseFourierAllowedNames();

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

    void applyFourierPeriodRangeFilter();

    void preCalc();
    void loadKnownPulsars();

    Data dispersionGenerateData();

    void closeEvent(QCloseEvent *);

private slots:
    void apply();
    void init();
    void loadFourierData(bool cashOnly = false);
    void calculateCashes();
    void applyFourierFilters();

    void actualFourierDataChanged();

    void dispersionPlot();
    void dispersionRemember();
    void dispersionMplus();

    void addPulsarCatalog();
    void showInfo();
    void knownPulsarsGUI();
};

#endif // ANALYTICS_H
