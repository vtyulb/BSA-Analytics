#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>
#include <QProgressBar>
#include <QByteArray>
#include <QObject>

#include <data.h>

class MainWindow;

enum Stair {
    NoStair = 0,
    SettingStair = 1,
    DetectedStair = 2
};

enum SourceMode {
    NoSourceMode = 0,
    RotationMeasure = 1,
    FluxDensity = 2
};

#ifdef WIN32
    const QString DOCPATH = ".";
#else
    const QString DOCPATH = "/usr/share/doc/bsa-analytics";
#endif

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
    bool periodTester();
    QProgressBar *getProgressBar();
    double dispersion();
    QVector<double> dispersionData();
    QVector<double> profileData(int dispersion);
    Data lastData();
    int stairStatus();

    int module();
    int ray();
    double period();
    bool fourierAnalytics();
    double getFourierStepConstant();
    int getFourierSpectreSize();
    bool getFourierHighGround();
    MainWindow *getMainWindow();

    double getStairHeight(int module, int ray, int channel);
    SourceMode sourceMode();

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
    void setLongRoads(bool);
    void setFourierAnalytics(bool);
    void setFourierStepConstant(double);
    void setFourierSpectreSize(int);
    void setFourierHighGround(bool);

    void setMainWindow(MainWindow*);

    void setStairStatus(Stair);
    void setSourceMode(SourceMode);
    QString stairFileName();
    void saveStair();
    bool loadStair();

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
    bool _singlePeriod;
    bool _noMultiplePeriods;
    bool _doNotClearNoise;
    bool _longRoads;
    QTime _time;
    double _period;
    double _dispersion;
    bool _fourierAnalytics;
    double _fourierStepConstant;
    int _fourierSpectreSize;
    bool _fourierHighGround;
    Stair _stairStatus;
    QString _stairFileName;
    SourceMode _sourceMode;
    QVector<double> dispersionPlotData;
    QVector<double> profilePlotData[200];
    QVector<QVector<QVector<double> > > stairs;
    Data _lastData;
    MainWindow *_mainWindow;
    QProgressBar *bar;
    int _currentProgress;

public slots:
    void setProgress(int);
};

#endif // SETTINGS_H
