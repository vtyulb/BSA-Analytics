#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>
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
        int dispersion();
        QVector<double> dispersionData();
        Data lastData();

        int module();
        int ray();
        double period();

        double getStairHeight(int module, int ray, int channel);
        bool sourceMode();

        void setSkipCount(int);
        void setSubZero(bool);
        void setIntellectualFilter(bool);
        void setLowMemoryMode(bool);
        void setRealOneStep(double);
        void setSoundMode(bool);
        void setSinglePeriod(bool);
        void setDoNotClearNoise(bool);
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

    private:
        Settings();

        bool filter;
        int _skipCount;
        bool _subZero;
        double _realOneStep;
        bool _lowMemory;
        bool _preciseSearch;
        bool _soundMode;
        int _ray;
        int _module;
        bool _flowFinder;
        bool _singlePeriod;
        bool _noMultiplePeriods;
        bool _doNotClearNoise;
        QTime _time;
        double _period;
        int _dispersion;
        QVector<double> dispersionPlotData;
        QVector<QVector<QVector<double> > > stairs;
        Data _lastData;
};

#endif // SETTINGS_H
