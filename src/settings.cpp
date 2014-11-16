#include "settings.h"

Settings::Settings() {
    filter = true;
}

Settings *Settings::settings() {
    static Settings *res = new Settings();
    return res;
}

bool Settings::intellectualFilter() {
    return filter;
}

void Settings::setIntellectualFilter(bool f) {
    filter = f;
}
