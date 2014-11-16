#ifndef SETTINGS_H
#define SETTINGS_H

class Settings {
    public:
        Settings();

        static Settings *settings();
        bool intellectualFilter();
        void setIntellectualFilter(bool);

    private:
        bool filter;

};

#endif // SETTINGS_H
