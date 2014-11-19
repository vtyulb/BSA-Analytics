#include "settings.h"

Settings::Settings() {
    filter = true;
    _skipCount = 0;
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

void Settings::setSkipCount(int skip) {
    _skipCount = skip;
}

int Settings::skipCount() {
    if (_skipCount)
        return _skipCount--;
    else
        return _skipCount;
}
