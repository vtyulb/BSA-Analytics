#include <settings.h>
#include <reader.h>

#include <QVariant>
#include <QSettings>

Settings::Settings() {
    filter = true;
    _skipCount = 0;
    _lowMemory = false;
    _preciseSearch = false;
    _soundMode = false;
    _noMultiplePeriods = false;
    _dispersion = -1;
    _doNotClearNoise = false;
    _singlePeriod = false;
    _longRoads = false;
    _periodTester = false;
    _fourierAnalytics = false;
    _fourierStepConstant = 0.1;
    _fourierSpectreSize = 1024;
    _fourierHighGround = true;
    _stairStatus = NoStair;
    bar = NULL;
}

Settings *Settings::settings() {
    static Settings *res = new Settings();
    return res;
}

void Settings::setPeriodTester(bool b) {
    _periodTester = b;
}

bool Settings::periodTester() {
    return _periodTester;
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

SourceMode Settings::sourceMode() {
    if (stairs.size() == 0)
        return NoSourceMode;
    else
        return _sourceMode;
}

void Settings::setSourceMode(SourceMode mode) {
    _sourceMode = mode;
}

void Settings::detectStair(const Data &data, int pointStart, int pointEnd) {
    stairs.resize(data.modules);
    for (int i = 0; i < data.modules; i++) {
        stairs[i].resize(data.rays);
        for (int j = 0; j < data.rays; j++)
            for (int k = 0; k < data.channels; k++) {
                QVector<double> tmp;
                for (int q = pointStart; q < pointEnd; q++)
                    tmp.push_back(data.data[i][k][j][q]);

                std::sort(tmp.begin(), tmp.end());

                stairs[i][j].push_back(tmp[tmp.size() - 5] - tmp[5]);
            }
    }

    _stairFileName = data.name;
    _lastData = data;
    saveStair();
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
    if (sourceMode())
        loadStair();
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

void Settings::setProgressBar(QProgressBar *_bar) {
    bar = _bar;
}

QProgressBar *Settings::getProgressBar() {
    return bar;
}

void Settings::setFourierAnalytics(bool b) {
    _fourierAnalytics = b;
}

bool Settings::fourierAnalytics() {
    return _fourierAnalytics;
}

void Settings::setFourierStepConstant(double d) {
    _fourierStepConstant = d;
}

double Settings::getFourierStepConstant() {
    return _fourierStepConstant;
}

void Settings::setFourierSpectreSize(int a) {
    _fourierSpectreSize = a;
}

int Settings::getFourierSpectreSize() {
    return _fourierSpectreSize;
}

void Settings::setFourierHighGround(bool a) {
    _fourierHighGround = a;
}

bool Settings::getFourierHighGround() {
    return _fourierHighGround;
}

void Settings::setStairStatus(Stair status) {
    _stairStatus = status;
    if (status == NoStair)
        stairs.clear();
}

int Settings::stairStatus() {
    return _stairStatus;
}

void Settings::saveStair() {
    QList<QVariant> stairList;
    for (int i = 0; i < stairs.size(); i++)
        for (int j = 0; j < stairs[0].size(); j++)
            for (int k = 0; k < stairs[0][0].size(); k++)
                stairList.push_back(stairs[i][j][k]);

    if (lastData().isLong()) {
        QSettings().setValue("LongStair", stairList);
        QSettings().setValue("LongStairName", lastData().name);
    } else {
        QSettings().setValue("ShortStair", stairList);
        QSettings().setValue("ShortStairName", lastData().name);
    }
}

bool Settings::loadStair() {
    QList<QVariant> stairList;
    if (lastData().isLong()) {
        stairList = QSettings().value("LongStair").toList();
        _stairFileName = QSettings().value("LongStairName").toString();
    } else {
        stairList = QSettings().value("ShortStair").toList();
        _stairFileName = QSettings().value("ShortStairName").toString();
    }

    if (stairList.isEmpty()) {
        _stairStatus = NoStair;
        return false;
    } else
        _stairStatus = DetectedStair;

    QList<QVariant>::Iterator current = stairList.begin();
    stairs.clear();
    stairs.resize(lastData().modules);
    for (int i = 0; i < stairs.size(); i++) {
        stairs[i].resize(lastData().rays);
        for (int j = 0; j < lastData().rays; j++)
            for (int k = 0; k < lastData().channels; k++)
            stairs[i][j].push_back((current++)->toDouble());
    }

    return true;
}

QString Settings::stairFileName() {
    return _stairFileName;
}
