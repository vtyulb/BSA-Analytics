#include <settings.h>
#include <reader.h>
#include <mainwindow.h>

#include <QVariant>
#include <QSettings>
#include <QMessageBox>

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
    _sourceMode = NoSourceMode,
    bar = NULL;
    _mainWindow = NULL;
    _currentProgress = -1;
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

void Settings::setDispersion(double d) {
    _dispersion = d + 1e-9;
}

double Settings::dispersion() {
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
    stairs.clear();
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
}

double Settings::getStairHeight(int module, int ray, int channel) {
    if (stairs.size())
        return stairs[module][channel][ray];
    else
        return 1;
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

void Settings::setProfileData(const QVector<double> &profile, int dispersion) {
    if (dispersion < 0 || dispersion >= 200)
        qDebug() << "error setting profile data";
    else
        profilePlotData[dispersion] = profile;
}

QVector<double> Settings::profileData(int dispersion) {
    if (dispersion < 0 || dispersion >= 200) {
        qDebug() << "error checking profile data";
        return QVector<double>();
    } else
        return profilePlotData[dispersion];
}

void Settings::setLastData(const Data &d) {
    _lastData = d;
    if (sourceMode())
        loadStair();
}

void Settings::setLastHeader(const QMap<QString, QString> &m) {
    _lastHeader = m;
}

void Settings::setSinglePeriod(bool b) {
    _singlePeriod = b;
}

bool Settings::singlePeriod() {
    return _singlePeriod;
}

Data Settings::getLastData() {
    return _lastData;
}

QMap<QString, QString> Settings::getLastHeader() {
    return _lastHeader;
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

bool Settings::loadStair() {
    static QString lastStairFor = "void";
    static Data shortStairs; static QStringList shortStairsNames;
    static Data longStairs; static QStringList longStairsNames;
    if (!getLastData().isValid())
        return false;

    if (lastStairFor != getLastData().name) {
        if (!longStairs.isValid()) {
            Data backup = getLastData();
            Reader r;
            QObject::connect(&r, SIGNAL(progress(int)), this, SLOT(setProgress(int)));
            longStairs = r.readBinaryFile(LONG_STAIRS);
            longStairsNames = getLastHeader()["stairs_names"].split(",");
            shortStairs = r.readBinaryFile(SHORT_STAIRS);
            shortStairsNames = getLastHeader()["stairs_names"].split(",");
            if (!longStairs.isValid() || !shortStairs.isValid()) {
                QMessageBox::warning(NULL, "Error: No stairs files found",
                                     "There are no stair files found!\n"
                                     "Program will NOT normalize data correctly,\n"
                                     "some functions may misbehave.\n\n"
                                     "You should install BSA-Analytics-stairs-pack\n"
                                     "from bsa.vtyulb.ru. If you have already done it,\n"
                                     "contact <vtyulb@vtyulb.ru> for further instructions.\n\n"
                                     "No further warnings may be issued at this session\n"
                                     "and it does not mean that problem is resolved.");
                return false;
            }
            setLastData(backup);
            _lastHeader.clear();
        }

        Data *actual;
        QStringList *names;
        if (getLastData().isLong()) {
            actual = &longStairs;
            names = &longStairsNames;
        } else {
            actual = &shortStairs;
            names = &shortStairsNames;
        }

        QDateTime me(stringDateToDate(getLastData().name));
        lastStairFor = getLastData().name;
        int current;
        for (current = 1; current < actual->npoints - 1; current++)
            if (me.secsTo(stringDateToDate(names->at(current))) >= 0)
                break;

        long long before = me.secsTo(stringDateToDate(names->at(current-1)));
        long long after = me.secsTo(stringDateToDate(names->at(current)));

        stairs.clear();
        stairs.resize(getLastData().modules);
        for (int module = 0; module < getLastData().modules; module++) {
            stairs[module].resize(getLastData().channels);
            for (int channel = 0; channel < getLastData().channels; channel++)
                for (int ray = 0; ray < getLastData().rays; ray++)
                    stairs[module][channel].push_back(
                                    actual->data[module][channel][ray][current-1] * (1 - (-before) / double(after-before))+
                                    actual->data[module][channel][ray][ current ] * (1 - ( after ) / double(after-before)));
        }

        _stairFileName = names->at(current-1).left(9) + "/" + names->at(current).left(12);

        return after < 3600*12 && before > -3600*12;
    }

    return true;
}

QString Settings::stairFileName() {
    return _stairFileName;
}

void Settings::setMainWindow(MainWindow *window) {
    _mainWindow = window;
}

MainWindow *Settings::getMainWindow() {
    if (!_mainWindow)
        _mainWindow = new MainWindow("void");
    return _mainWindow;
}

void Settings::setProgress(int progress) {
    if (!bar) {
        if (_currentProgress != progress) {
            printf(".");
            if (progress == 100)
                printf("\n");

            fflush(stdout);
            _currentProgress = progress;
        }
    } else {
        if (progress == 100)
            bar->hide();
        else {
            bar->show();
            bar->setValue(progress);
        }
    }
}
