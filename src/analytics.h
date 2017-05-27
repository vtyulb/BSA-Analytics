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
    explicit Analytics(QString analyticsPath, bool fourier = false, QWidget *parent = 0);
    ~Analytics();

private:
    Ui::Analytics *ui;
    PulsarList *list;
    MainWindow *window;
    KnownNoise *knownNoises;
    QString folder;
    QStringList catalogs;
    bool oneWindowMode;

    Data noises;
    QMap<QString, QString> noisesHeader;

    QProgressBar *progressBar;

    Pulsars pulsars;
    QVector<bool> pulsarsEnabled;
    QVector<bool> differentNoisePreCalc;
    QVector<bool> powerfullNoisePreCalc;


    int maxModule;
    int maxRay;
    bool fourier;
    bool transient;
    bool transientWhitezoneEnabled = true;
    int fourierSpectreSize;
    bool longData;
    int totalFilesLoaded;
    bool cacheLoaded;

    QVector<KnownPulsar> knownPulsars;
    QVector<QString> fileNames;
    QVector<QDate> fourierAllowedDates;
    QVector<Data> fourierData;
    QVector<QMap<QString, QString> > headers;
    QVector<float> fourierSumm[6][8];
    QVector<double> fourierRawNoises[6][8];
    QVector<double> fourierNoises[6][8];
    QVector<bool> fourierGood;

    void parseFourierAllowedDates();

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
    void applyFourierKnownNoiseFilter();
    void applyKnownNoiseFilter();
    void applyFileNameFilter();
    void applyDifferentMaximumsFilter();

    void destroyGPS(Data &spectre);
    void destroyPeak(Data &spectre, int point);

    void buildTransientWhitezone(Pulsars &res);

    void preciseDataMode();
    void compressLayout();

    void preCalc();
    void loadKnownPulsars();

    Data dispersionGenerateData();

private slots:
    void apply(bool fullFilters = true);
    void init();
    void loadFourierData(bool cacheOnly = false, bool loadCache = false);
    void loadFourierCache();
    void calculateCaches();

    void applyFourierFilters();
    void applyTransientFilters();

    void findTransientPeriod();

    void actualFourierDataChanged();
    void fourierShowSpectresNoise();
    void fourierShowNoises();
    bool fourierLoadNoises();
    void fourierSelectBestAuto();
    void fourierSelectBestEnabled(bool);

    void fourierFullGrayZone();
    void fourierShortGrayZone();

    void applyTransientLoneObjects();
    void applyTransientMultipleRaysFilter();
    void applyTransientTrashDays();

    void enableTransientWhitezone(bool);

    void dispersionPlot();
    void dispersionRemember();
    void dispersionMplus();

    void profileRemember();
    void profileMplus();

    void addPulsarCatalog();
    void showInfo();
    void knownPulsarsGUI();
    void oneWindow();

    void transientSaveImage(bool forPublication = false);
    void transientSaveImageForPublication();
};

#endif // ANALYTICS_H
