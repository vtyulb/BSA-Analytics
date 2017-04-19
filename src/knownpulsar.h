#ifndef KNOWNPULSAR
#define KNOWNPULSAR

#include <QTime>
#include <QString>
#include <QVariant>

#include <pulsar.h>
#include <settings.h>

const QString KNOWN_PULSARS_FILENAME = "/known-pulsars.pls";

struct KnownPulsar {
    KnownPulsar() {};
    KnownPulsar(QVariant data) {
        QList<QVariant> res = data.toList();
        module = res[0].toInt();
        ray = res[1].toInt();
        period = res[2].toDouble();
        time = res[3].toTime();
        comment = res[4].toString();
        shortNoise = res[5].toBool();
        longNoise = res[6].toBool();
    }

    KnownPulsar(int _module, int _ray, double _period, QTime _time, QString _comment):
        module(_module),
        ray(_ray),
        period(_period),
        time(_time),
        comment(_comment)
    {};

    int module;
    int ray;
    double period;
    QTime time;
    QString comment;

    bool shortNoise = false;
    bool longNoise = false;

    bool operator == (const Pulsar &p) const {
        return (module == p.module || module == -1) &&
                (ray == p.ray || ray == -1) &&
                (goodPeriods(period, p.period) || period < 0) &&
                (abs(p.nativeTime.secsTo(time)) <= 180  || time.isNull());
    }

    bool goodPeriods(double a, double b) const {
        double prec = 0.001;
        if (Settings::settings()->getFourierSpectreSize() != 1024)
            prec /= 8;

        return fabs(a - b) < prec;
    }

    QVariant toVariant() {
        QList<QVariant> res;
        res.push_back(module);
        res.push_back(ray);
        res.push_back(period);
        res.push_back(time);
        res.push_back(comment);
        res.push_back(shortNoise);
        res.push_back(longNoise);
        return res;
    }
};

#endif // KNOWNPULSAR

