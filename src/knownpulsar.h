#ifndef KNOWNPULSAR
#define KNOWNPULSAR

#include <QTime>
#include <QString>
#include <QVariant>

#include <pulsar.h>

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

    bool operator == (const Pulsar &p) const {
        return (module == p.module) &&
                (ray == p.ray) &&
                (globalGoodDoubles(period, p.period)) &&
                abs(p.nativeTime.secsTo(time)) <= 180;
    }

    QVariant toVariant() {
        QList<QVariant> res;
        res.push_back(module);
        res.push_back(ray);
        res.push_back(period);
        res.push_back(time);
        res.push_back(comment);;
        return res;
    }
};

#endif // KNOWNPULSAR

