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
                (goodDoubles(period, p.period)) &&
                abs(p.nativeTime.secsTo(time)) <= 120;
    }

    bool goodDoubles(double a, double b) const {
        if (a > b)
            a /= b;
        else
            a = b / a;

        a = fabs(a - int(a + 0.5));

        return interval * a < 1.01 * b;
    }
};

#endif // KNOWNPULSAR

