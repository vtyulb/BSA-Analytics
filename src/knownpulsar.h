#ifndef KNOWNPULSAR
#define KNOWNPULSAR

#include <QTime>
#include <QString>
#include <pulsar.h>

const QString KNOWN_PULSARS_FILENAME = "/known-pulsars.pls";

struct KnownPulsar {
    KnownPulsar() {};
    KnownPulsar(int _module, int _ray, double _period, QTime _time):
        module(_module),
        ray(_ray),
        period(_period),
        time(_time)
    {};

    int module;
    int ray;
    double period;
    QTime time;

    bool operator == (const Pulsar &p) const {
        return (module == p.module) &&
                (ray == p.ray) &&
                (globalGoodDoubles(period, p.period)) &&
                abs(p.nativeTime.secsTo(time)) <= 120;
    }
};

#endif // KNOWNPULSAR

