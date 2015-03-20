#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>
#include <QTime>

class Settings {
    public:
        Settings();

        static Settings *settings();

        bool intellectualFilter();
        bool subZero();
        bool lowMemory();
        bool preciseSearch();
        int skipCount();
        double realOneStep();
        bool soundMode();

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
        void detectStair(char *name, int point);
        QTime getTime();

        void setPreciseSearch(bool);
        void setModule(int);
        void setRay(int);
        void setPeriod(double);
        void setTime(QTime);

    private:
        bool filter;
        int _skipCount;
        bool _subZero;
        double _realOneStep;
        bool _lowMemory;
        bool _preciseSearch;
        bool _soundMode;
        int _ray;
        int _module;
        QTime _time;
        double _period;
        QVector<QVector<QVector<double> > > stairs;
};

#endif // SETTINGS_H
