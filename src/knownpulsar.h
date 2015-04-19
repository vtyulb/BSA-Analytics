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
        if (a < b) {
            double c = a;
            a = b;
            b = c;
        }

        if (a > 1.1 * b)
            return false;

        a = fabs(a - b);
        a = (a * interval / b);

        if (0.1 * b > 0.5)
            return a < 0.1 * b;
        else
            return a < 0.5;
    }
};

#endif // KNOWNPULSAR

