#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVector>

class Settings {
    public:
        Settings();

        static Settings *settings();

        bool intellectualFilter();
        bool subZero();
        bool lowMemory();
        int skipCount();
        double realOneStep();

        double getStairHeight(int module, int ray, int channel);
        bool sourceMode();

        void setSkipCount(int);
        void setSubZero(bool);
        void setIntellectualFilter(bool);
        void setLowMemoryMode(bool);
        void setRealOneStep(double);
        void detectStair(char *name, int point);

    private:
        bool filter;
        int _skipCount;
        bool _subZero;
        double _realOneStep;
        bool _lowMemory;
        QVector<QVector<QVector<double> > > stairs;
};

#endif // SETTINGS_H
