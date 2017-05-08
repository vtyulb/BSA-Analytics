#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>
#include <QProgressBar>
#include <QByteArray>
#include <QMap>
#include <QObject>

#include <data.h>
#include <spectredrawer.h>

class MainWindow;

enum SourceMode {
    NoSourceMode = 0,
    RotationMeasure = 1,
    FluxDensity = 2
};

#ifdef Q_OS_LINUX
    const QString DATA_PATH = "/usr/share/bsa-analytics";
    const QString DOC_PATH = "/usr/share/doc/bsa-analytics";

    const QString SHORT_STAIRS = DATA_PATH + "/ShortStairs.pnt";
    const QString LONG_STAIRS = DATA_PATH + "/LongStairs.pnthr";
#else
    const QString DOC_PATH = ".";

    const QString SHORT_STAIRS = "data/ShortStairs.pnt";
    const QString LONG_STAIRS = "data/LongStairs.pnthr";
#endif

const bool STABLE_VERSION = false;
const QString SHORT_NOISES = "/noises.pnt";
const QString LONG_NOISES = "/noises.pnthr";

class Settings: public QObject
{
    Q_OBJECT
public:
    static Settings *settings();
    bool intellectualFilter();
    bool subZero();
    bool lowMemory();
    bool preciseSearch();
    int skipCount();
    double realOneStep();
    bool soundMode();
    bool noMultiplePeriods();
    bool doNotClearNoise();
    bool singlePeriod();
    bool longRoads();
    bool nullOnOYaxis();
    bool periodTester();
    QProgressBar *getProgressBar();
    double dispersion();
    QVector<double> dispersionData();
    QVector<double> profileData(int dispersion);
    Data getLastData();
    QMap<QString, QString> getLastHeader();

    int module();
    int ray();
    double period();
    bool fourierAnalytics();
    bool transientAnalytics();
    double getFourierStepConstant();
    int getFourierSpectreSize();
    bool getFourierHighGround();
    MainWindow *getMainWindow();
    SpectreDrawer *getSpectreDrawer();
    bool getNormalize();

    double getStairHeight(int module, int ray, int channel);
    SourceMode sourceMode();

    void setNullOnOYaxis(bool);
    void setSkipCount(int);
    void setSubZero(bool);
    void setIntellectualFilter(bool);
    void setLowMemoryMode(bool);
    void setRealOneStep(double);
    void setSoundMode(bool);
    void setSinglePeriod(bool);
    void setPeriodTester(bool);
    void setDoNotClearNoise(bool);
    void setProgressBar(QProgressBar*);
    void detectStair(const Data &data, int pointStart, int pointEnd);
    QTime getTime();

    void setPreciseSearch(bool);
    void setModule(int);
    void setRay(int);
    void setPeriod(double);
    void setTime(QTime);
    void setNoMultiplePeriods(bool);
    void setDispersion(double);
    void setDispersionData(const QVector<double>&);
    void setProfileData(const QVector<double> &profile, int dispersion);
    void setLastData(const Data&);
    void setLastHeader(const QMap<QString, QString>&);
    void setLongRoads(bool);
    void setFourierAnalytics(bool);
    void setTransientAnalytics(bool);
    void setFourierStepConstant(double);
    void setFourierSpectreSize(int);
    void setFourierHighGround(bool);
    void setNormalize(bool);

    void setMainWindow(MainWindow*);
    void setSpectreDrawer(SpectreDrawer*);

    void setSourceMode(SourceMode);
    QString stairFileName();
    bool loadStair();

    QString version();

private:
    Settings();

    bool filter;
    int _skipCount;
    bool _subZero;
    double _realOneStep;
    bool _lowMemory;
    bool _preciseSearch;
    bool _soundMode;
    bool _periodTester;
    int _ray;
    int _module;
    bool _nullOnOYaxis;
    bool _singlePeriod;
    bool _noMultiplePeriods;
    bool _doNotClearNoise;
    bool _longRoads;
    QTime _time;
    double _period;
    double _dispersion;
    bool _fourierAnalytics;
    bool _transientAnalytics;
    double _fourierStepConstant;
    int _fourierSpectreSize;
    bool _fourierHighGround;
    bool _normalize;
    QString _stairFileName;
    SourceMode _sourceMode;
    QVector<double> dispersionPlotData;
    QVector<double> profilePlotData[200];
    QVector<QVector<QVector<double> > > stairs;
    Data _lastData;
    QMap<QString, QString> _lastHeader;
    MainWindow *_mainWindow;
    SpectreDrawer *_spectreDrawer;
    QProgressBar *bar;
    int _currentProgress;

public slots:
    void setProgress(int);
};

#endif // SETTINGS_H
