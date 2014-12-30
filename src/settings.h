#ifndef SETTINGS_H
#define SETTINGS_H

class Settings {
    public:
        Settings();

        static Settings *settings();

        bool intellectualFilter();
        bool subZero();
        int skipCount();
        double realOneStep();

        void setSkipCount(int);
        void setSubZero(bool);
        void setIntellectualFilter(bool);
        void setRealOneStep(double);

    private:
        bool filter;
        int _skipCount;
        bool _subZero;
        double _realOneStep;

};

#endif // SETTINGS_H
