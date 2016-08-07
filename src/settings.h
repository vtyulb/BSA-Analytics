#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>
#include <QProgressBar>
#include <data.h>

class Settings {
    public:
        static Settings *settings();

        bool intellectualFilter();
        bool subZero();
        bool lowMemory();
        bool preciseSearch();
        int skipCount();
        double realOneStep();
        bool soundMode();
        bool flowFinder();
        bool noMultiplePeriods();
        bool doNotClearNoise();
        bool singlePeriod();
        bool longRoads();
        bool periodTester();
        QProgressBar *getProgressBar();
        int dispersion();
        QVector<double> dispersionData();
        Data lastData();

        int module();
        int ray();
        double period();
        bool fourierAnalytics();

        double getStairHeight(int module, int ray, int channel);
        bool sourceMode();

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
        void detectStair(char *name, int point);
        QTime getTime();

        void setPreciseSearch(bool);
        void setModule(int);
        void setRay(int);
        void setPeriod(double);
        void setTime(QTime);
        void setFlowFinder(bool);
        void setNoMultiplePeriods(bool);
        void setDispersion(int);
        void setDispersionData(const QVector<double>&);
        void setLastData(const Data&);
        void setLongRoads(bool);
        void setFourierAnalytics(bool);

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
        bool _flowFinder;
        bool _singlePeriod;
        bool _noMultiplePeriods;
        bool _doNotClearNoise;
        bool _longRoads;
        QTime _time;
        double _period;
        int _dispersion;
        bool _fourierAnalytics;
        QVector<double> dispersionPlotData;
        QVector<QVector<QVector<double> > > stairs;
        Data _lastData;
        QProgressBar *bar;
};

#endif // SETTINGS_H
