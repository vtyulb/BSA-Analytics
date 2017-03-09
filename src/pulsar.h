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
#include <QTime>
#include <QDebug>

const double INTERVAL = 17;
const double MINIMUM_PERIOD = 0.5;
const double MAXIMUM_PERIOD = 10;
const double PERIOD_STEP = 0.01;
const double FOURIER_PULSAR_LEVEL_SNR = 1.5;

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
    double sigma;
    bool filtered;
    bool showInTable;

    bool badNoiseKnown = false;
    bool badNoiseRes;

    double noiseLevel;

    double fourierRealNoise = -1;
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

        if (period < 0.0001) {
            qDebug() << "really strange things are happening now: period" << period;
        } else {
            for (int offset = -period / data.oneStep / 2; offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1 + additionalSize; offset++) {
                double sum = 0;
                int n = 0;
                for (double i = firstPoint + offset; i < firstPoint + offset + interval / data.oneStep; i += period / data.oneStep * 2, n++)
                    sum += disp[int(i)];

                d.push_back(sum / n);
            }
        }

        for (int i = 0; i < 100; i++)
            d.push_back(0);

        if (period >= 0.0001) {
            for (int i = firstPoint; i < firstPoint + interval / data.oneStep; i++)
                d.push_back(disp[i]);
        }

        additionalData.clear();
        QDataStream stream(&additionalData, QIODevice::WriteOnly);
        stream << QVariant(d.toList());
    }

    void findFourierData(int startPoint) {
        int st = 50;
        int ls = data.npoints - 2;
        double noise;
        float avr;
        if (fourierRealNoise < 0) {
            QVector<float> ns;
            for (int i = st; i < ls; i++)
                ns.push_back(data.data[0][0][0][i]);

            std::sort(ns.begin(), ns.end());
            int stt = 20;
            int lst = ns.size() * 0.7;

            avr = ns[ns.size() * 0.5];

            noise = 0;
            for (int i = stt; i < lst; i++)
                noise += pow(ns[i] - avr, 2);

            noise /= (lst - stt);
            noise = pow(noise, 0.5);

            noise *= 2;

            fourierRealNoise = noise;
            fourierAverage = avr;

            noiseLevel = 0;
            for (int i = 0; i < data.npoints; i++)
                noiseLevel += pow(data.data[0][0][0][i]-avr, 2);
            noiseLevel /= data.npoints;
            noiseLevel = pow(noiseLevel, 0.5);

            dispersion = noiseLevel * 1000000;
        } else {
            avr = fourierAverage;
            noise = fourierRealNoise;
        }

        float currentMin = 1e+10;
        for (int i = startPoint; i < ls; i++) {
            float mx = data.data[0][0][0][i];
            if (i > 20)
                currentMin = std::min(currentMin, mx);

            snr = (mx-std::max(avr, currentMin+float(noise)))/noise;
            if (snr > FOURIER_PULSAR_LEVEL_SNR) {
                firstPoint = i + 1;
                period = Settings::settings()->getFourierSpectreSize() * 2 / double(i + 1) * Settings::settings()->getFourierStepConstant();

                if (snr > FOURIER_PULSAR_LEVEL_SNR && data.data[0][0][0][i + 1] < mx && data.data[0][0][0][i - 1] < mx)
                    break;
            } else
                snr = -555;
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

static inline bool globalGoodDoubles(double a, double b, bool twice = false) {
    if (a < b)
        std::swap(a, b);

    if (a > (1.1 + twice) * b)
        return false;

    if (a > 1.9 * b)
        a /= 2;

    return fabs(a - b) < 0.005;
}

static inline QString getPulsarJName(int module, int ray, QTime time) {
    module--;
    ray--;
    static const char *degree[6][8] = {
        {"4213", "4172", "4131", "4089", "4047", "4006", "3964", "3923"},
        {"3879", "3838", "3795", "3754", "3711", "3669", "3626", "3585"},
        {"3540", "3497", "3454", "3412", "3369", "3325", "3282", "3238"},
        {"3194", "3150", "3106", "3061", "3017", "2973", "2929", "2884"},
        {"2837", "2792", "2747", "2701", "2656", "2610", "2564", "2518"},
        {"2470", "2423", "2376", "2329", "2281", "2234", "2186", "2138"}
    };

    static const char *lowDegree[6][8] = {
        {"2083", "2039", "1989", "1941", "1891", "1841", "1791", "1740"},
        {"1687", "1636", "1584", "1532", "1480", "1427", "1374", "1320"},
        {"1263", "1210", "1154", "1098", "1042", "0985", "0928", "0870"},
        {"0809", "0750", "0690", "0629", "0568", "0505", "0442", "0378"},
        {"0311", "0245", "0179", "0111", "0042", "-028", "-100", "-172"},
        {"-250", "-325", "-402", "-482", "-562", "-646", "-732", "-820"}
    };

    if (module < 0)
        module = 0;
    else if (module > 5)
        module = 5;

    if (ray < 0)
        ray = 0;
    else if (ray > 7)
        ray = 7;

    QString numb = degree[module][ray];
    if (!Settings::settings()->getFourierHighGround())
        numb = lowDegree[module][ray];

    int minutes = numb.right(2).toInt();
    minutes = minutes * 60 / 100;

    return QString("J" + time.toString("HHmm") + (numb[0] == '-' ? "" : "+") + numb.left(2) + QString::number(minutes / 10) + QString::number(minutes % 10)).replace("-", "-0");
}

typedef QVector<Pulsar>* Pulsars;


#endif // PULSAR_H
