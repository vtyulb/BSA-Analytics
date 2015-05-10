#include "settings.h"
#include <reader.h>

Settings::Settings() {
    filter = true;
    _skipCount = 0;
    _lowMemory = false;
    _preciseSearch = false;
    _soundMode = false;
    _flowFinder = false;
    _noMultiplePeriods = false;
    _dispersion = -1;
    _doNotClearNoise = false;
    _singlePeriod = false;
    _longRoads = false;
}

Settings *Settings::settings() {
    static Settings *res = new Settings();
    return res;
}

void Settings::setLongRoads(bool b) {
    _longRoads = b;
}

bool Settings::longRoads() {
    return _longRoads;
}

void Settings::setDispersion(int d) {
    _dispersion = d;
}

int Settings::dispersion() {
    return _dispersion;
}

void Settings::setNoMultiplePeriods(bool b) {
    _noMultiplePeriods = b;
}

bool Settings::noMultiplePeriods() {
    return _noMultiplePeriods;
}

bool Settings::soundMode() {
    return _soundMode;
}

void Settings::setSoundMode(bool s) {
    _soundMode = s;
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

bool Settings::lowMemory() {
    return _lowMemory;
}

void Settings::setLowMemoryMode(bool b) {
    _lowMemory = b;
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
            for (int k = 0; k < data.channels; k++) {
                double sum = 0;
                for (int q = -10; q < 10; q++)
                    sum += data.data[i][k][j][point + q];

                std::sort(data.data[i][k][j], data.data[i][k][j] + data.npoints);
                double sum1 = 0;
                for (int q = 0; q < 20; q++)
                    sum1 += data.data[i][k][j][q];

                stairs[i][j].push_back(sum / 20 - sum1 / 20);
            }
    }

    data.releaseData();
}

double Settings::getStairHeight(int module, int ray, int channel) {
    return stairs[module][ray][channel];
}

void Settings::setPreciseSearch(bool p) {
    _preciseSearch = p;
}

bool Settings::preciseSearch() {
    return _preciseSearch;
}

void Settings::setModule(int m) {
    _module = m;
}

void Settings::setRay(int r) {
    _ray = r;
}

void Settings::setPeriod(double p) {
    _period = p;
}

int Settings::module() {
    return _module;
}

int Settings::ray() {
    return _ray;
}

double Settings::period() {
    return _period;
}

void Settings::setTime(QTime t) {
    _time = t;
}

QTime Settings::getTime() {
    return _time;
}

void Settings::setFlowFinder(bool f) {
    _flowFinder = f;
}

bool Settings::flowFinder() {
    return _flowFinder;
}

void Settings::setDoNotClearNoise(bool b) {
    _doNotClearNoise = b;
}

bool Settings::doNotClearNoise() {
    return _doNotClearNoise;
}

void Settings::setDispersionData(const QVector<double> &data) {
    dispersionPlotData = data;
}

QVector<double> Settings::dispersionData() {
    return dispersionPlotData;
}

void Settings::setLastData(const Data &d) {
    _lastData = d;
}

void Settings::setSinglePeriod(bool b) {
    _singlePeriod = b;
}

bool Settings::singlePeriod() {
    return _singlePeriod;
}

Data Settings::lastData() {
    return _lastData;
}
