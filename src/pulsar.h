#ifndef PULSAR_H
#define PULSAR_H

#include <data.h>
#include <startime.h>
#include <settings.h>
#include <math.h>

#include <QString>
#include <QVector>
#include <QDataStream>
#include <QVariant>

const double INTERVAL = 17;
const double MINIMUM_PERIOD = 0.5;
const double MAXIMUM_PERIOD = 10;
const double PERIOD_STEP = 0.01;

const int interval = 180;
const int CATEGORIES = 4;
const int CATEGORIES_SIZES[CATEGORIES + 1] = {4, 5, 7, 10, 1000};

struct Pulsar {
    Data data;
    int module;
    int ray;
    int dispersion;
    int firstPoint;
    double period; // in seconds;
    double snr;
    bool filtered;

    bool badNoiseKnown = false;
    bool badNoiseRes;
    bool similiarPeaks;

    double noiseLevel;

    double fourierRealNoise;
    float fourierAverage;
    bool fourierDuplicate = false;

    QString name; // file, not a pulsar :-)
    QByteArray additionalData;
    bool valid;
    QTime nativeTime;

    QString print() {
        return QString("Pulsar in %1 module %2, ray %3, D%4, frstp %5 %6, period %7, snr %8").
                arg(name).
                arg(QString::number(module + 1)).
                arg(QString::number(ray + 1)).
                arg(QString::number(dispersion)).
                arg(QString::number(firstPoint)).
                arg(time()).
                arg(QString::number(period)).
                arg(QString::number(snr));
    }

    QString time() const {
        return StarTime::StarTime(data, firstPoint + interval / 2 / data.oneStep);
    }

    QString UTCtime() const {
        double oneStep = data.name.contains(".pnthr") ? 0.0124928 : 0.0999424;
        QTime start((data.name[7] + QString(data.name[8])).toInt(), 0);
        QDate date((QString(data.name[4])+data.name[5]).toInt(),
                   (QString(data.name[2])+data.name[3]).toInt(),
                   (QString(data.name[0])+data.name[1]).toInt());

        QDateTime res(date, start);
        res = res.addSecs(-3600*3);
        res = res.addMSecs(firstPoint*oneStep*1000);
        return "UTC " + res.toString("dd.MM.yy HH:mm:ss.zzz");
    }

    void calculateAdditionalData(const QVector<double> &disp) {
        QVector<QVariant> d;
        int additionalSize = 0;
        if (Settings::settings()->longRoads())
            additionalSize = 1000;

        for (int offset = -period / data.oneStep / 2; offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1 + additionalSize; offset++) {
            double sum = 0;
            int n = 0;
            for (double i = firstPoint + offset; i < firstPoint + offset + interval / data.oneStep; i += period / data.oneStep * 2, n++)
                sum += disp[int(i)];

            d.push_back(sum / n);
        }

        float m1 = 0.00001;
        float m2 = 0.00001;
        for (int i = 0; i < d.size() / 2; i++)
            if (m1 < d[i].toFloat())
                m1 = d[i].toFloat();

        for (int i = period / data.oneStep / 2 * 3 - 1; i < period / data.oneStep / 2 * 3 + 2; i++)
            if (m2 < d[i].toFloat())
                m2 = d[i].toFloat();

        if (m1 < m2)
            std::swap(m1, m2);

        similiarPeaks = m2 * 1.2 > m1;

        for (int i = 0; i < 100; i++)
            d.push_back(0);

        for (int i = firstPoint; i < firstPoint + interval / data.oneStep; i++)
            d.push_back(disp[i]);

        additionalData.clear();
        QDataStream stream(&additionalData, QIODevice::WriteOnly);
        stream << QVariant(d.toList());
    }

    void findFourierData(int startPoint) {
        int st = 50;
        int ls = Settings::settings()->getFourierSpectreSize() - 1;
        QVector<float> ns;
        for (int i = st; i < ls; i++)
            ns.push_back(data.data[0][0][0][i]);

        std::sort(ns.begin(), ns.end());
        int stt = ns.size() * 0.3;
        int lst = ns.size() * 0.7;

        float avr = ns[ns.size() / 2];

        double noise = 0;
        for (int i = stt; i < lst; i++)
            noise += pow(ns[i] - avr, 2);

        noise /= (lst - stt);
        noise = pow(noise, 0.5);

        noise *= 2;

        fourierRealNoise = noise;
        fourierAverage = avr;

        noiseLevel = 0;
        for (int i = 0; i < Settings::settings()->getFourierSpectreSize(); i++)
            noiseLevel += pow(data.data[0][0][0][i], 2);
        noiseLevel /= Settings::settings()->getFourierSpectreSize();
        noiseLevel = pow(noiseLevel, 0.5);

        dispersion = noiseLevel * 1000000;

        float mx = 0;
        for (int i = startPoint; i < ls; i++)
            if (data.data[0][0][0][i] > mx) {
                mx = data.data[0][0][0][i];
                snr = (mx-avr)/noise;
                firstPoint = i + 1;
                period = Settings::settings()->getFourierSpectreSize() * 2 / double(i + 1) * Settings::settings()->getFourierStepConstant();

                if (snr > 5 && data.data[0][0][0][i + 1] < mx)
                    break;
            }
    }

    void squeeze() {
        int i = 0;
        while (data.data[0][0][0][i] != 0)
            i++;

        data.npoints = i + 10;
        float *nd = new float[i + 10];
        memcpy(nd, data.data[0][0][0], sizeof(float) * (i + 10));

        delete[] data.data[0][0][0];
        data.data[0][0][0] = nd;
    }

    bool operator < (const Pulsar &p) const {        
        if (nativeTime.secsTo(p.nativeTime) > 0)
            return true;
        else if (nativeTime.secsTo(p.nativeTime) < 0)
            return false;

        if (dispersion < p.dispersion)
            return true;
        else if (dispersion > p.dispersion)
            return false;

        if (module < p.module)
            return true;
        else if (module > p.module)
            return false;

        if (ray < p.ray)
            return true;

        return false;
    }

    static bool secondComparator(const Pulsar &p1, const Pulsar &p2) {
        if (!p1.filtered && p2.filtered)
            return true;
        else if (p1.filtered && !p2.filtered)
            return false;

        return p1 < p2;
    }

    bool badNoise() const {
        if (badNoiseKnown)
            return badNoiseRes;

        int j = 0;
        while (data.data[0][0][0][j] != 0) j++;
        while (data.data[0][0][0][j] == 0) j++;

        QVector<double> sigmas;
        int n = data.npoints - j;
        float *dt = data.data[0][0][0] + j;
        bool res = true;
        const int pieces = 8;
        for (int k = 0; k < pieces; k++) {
            double sigma = 0;
            for (int i = k * n / pieces; i < (k + 1) * n / pieces; i++)
                sigma += dt[i] * dt[i];

            sigma /= (n / pieces);
            sigma = pow(sigma, 0.5);
            sigmas.push_back(sigma);
        }

        for (int i = 0; i < pieces; i++)
            for (int j = 0; j < pieces; j++)
                if (sigmas[i] / sigmas[j] > 3)
                    res = false;

        return !res;
    }

    void save(QDataStream &out) const {
        out << data.npoints << data.modules << data.channels;
        out << data.rays << data.time << data.previousLifeName;
        out << data.name << data.oneStep << data.delta_lucha;
        out << data.fbands;
        out << data.sigma << data.releaseProtected;
        for (int i = 0; i < data.npoints; i++)
            out << data.data[0][0][0][i];

        out << module << ray << dispersion;
        out << firstPoint << period << snr;
        out << filtered << badNoiseKnown;
        out << badNoiseRes << noiseLevel;
        out << name << additionalData << valid;
        out << nativeTime;
    }

    void load(QDataStream &in) {
        in >> data.npoints >> data.modules >> data.channels;
        in >> data.rays >> data.time >> data.previousLifeName;
        in >> data.name >> data.oneStep >> data.delta_lucha;
        in >> data.fbands;
        in >> data.sigma >> data.releaseProtected;
        double sg = data.sigma;
        data.init();
        data.sigma = sg;
        for (int i = 0; i < data.npoints; i++)
            in >> data.data[0][0][0][i];

        in >> module >> ray >> dispersion;
        in >> firstPoint >> period >> snr;
        in >> filtered >> badNoiseKnown;
        in >> badNoiseRes >> noiseLevel;
        in >> name >> additionalData >> valid;
        in >> nativeTime;
    }
};

static bool globalGoodDoubles(double a, double b, bool twice = false) {
    if (a < b) {
        double c = a;
        a = b;
        b = c;
    }

    if (a > (1.1 + twice) * b)
        return false;

    if (a > 1.9 * b)
        a /= 2;

    a = fabs(a - b);
    a = (a * interval / b);

    if (0.1 * b > 0.5)
        return a < 0.1 * b;
    else
        return a < 0.5;
}

typedef QVector<Pulsar>* Pulsars;


#endif // PULSAR_H
