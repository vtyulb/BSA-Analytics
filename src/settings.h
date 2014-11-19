#ifndef SETTINGS_H
#define SETTINGS_H

class Settings {
    public:
        Settings();

        static Settings *settings();

        bool intellectualFilter();
        void setIntellectualFilter(bool);

        int skipCount();
        void setSkipCount(int);

    private:
        bool filter;
        int _skipCount;

};

#endif // SETTINGS_H
