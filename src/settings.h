#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>
#include <QProgressBar>
#include <QByteArray>
#include <QMap>
#include <QObject>

#include <data.h>

class MainWindow;

enum SourceMode {
    NoSourceMode = 0,
    RotationMeasure = 1,
    FluxDensity = 2
};

#ifdef Q_OS_LINUX
    const QString DOC_PATH = "/usr/share/doc/bsa-analytics";
    const QString SHORT_STAIRS = "/usr/share/bsa-analytics/ShortStairs.pnt";
    const QString LONG_STAIRS = "/usr/share/bsa-analytics/LongStairs.pnthr";
#else
    const QString DOC_PATH = ".";
    const QString SHORT_STAIRS = "data/ShortStairs.pnt";
    const QString LONG_STAIRS = "data/LongStairs.pnthr";
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
    Data getLastData();
    QMap<QString, QString> getLastHeader();

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
    void setLastHeader(const QMap<QString, QString>&);
    void setLongRoads(bool);
    void setFourierAnalytics(bool);
    void setFourierStepConstant(double);
    void setFourierSpectreSize(int);
    void setFourierHighGround(bool);

    void setMainWindow(MainWindow*);

    void setSourceMode(SourceMode);
    QString stairFileName();
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
    QString _stairFileName;
    SourceMode _sourceMode;
    QVector<double> dispersionPlotData;
    QVector<double> profilePlotData[200];
    QVector<QVector<QVector<double> > > stairs;
    Data _lastData;
    QMap<QString, QString> _lastHeader;
    MainWindow *_mainWindow;
    QProgressBar *bar;
    int _currentProgress;

public slots:
    void setProgress(int);
};

#endif // SETTINGS_H
