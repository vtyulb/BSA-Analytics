#include "settings.h"
#include <reader.h>

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

void Settings::setSubZero(bool b) {
    _subZero = b;
}

bool Settings::subZero() {
    return _subZero;
}

double Settings::realOneStep() {
    return _realOneStep;
}

void Settings::setRealOneStep(double st) {
    _realOneStep = st;
}

bool Settings::sourceMode() {
    return stairs.size() > 0;
}

void Settings::detectStair(char *name, int point) {
    Reader reader;
    Data data = reader.readBinaryFile(QString(name));
    stairs.resize(data.modules);
    for (int i = 0; i < data.modules; i++) {
        stairs[i].resize(data.rays);
        for (int j = 0; j < data.rays; j++)
            for (int k = 0; k < data.channels; k++)
                stairs[i][j].push_back(data.data[i][k][j][point]);
    }

    data.releaseData();
}

double Settings::getStairHeight(int module, int ray, int channel) {
    return stairs[module][ray][channel];
}
