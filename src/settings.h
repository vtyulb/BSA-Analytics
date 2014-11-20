#ifndef SETTINGS_H
#define SETTINGS_H

class Settings {
    public:
        Settings();

        static Settings *settings();

        bool intellectualFilter();
        bool subZero();
        int skipCount();

        void setSkipCount(int);
        void setSubZero(bool);
        void setIntellectualFilter(bool);

    private:
        bool filter;
        int _skipCount;
        bool _subZero;

};

#endif // SETTINGS_H
