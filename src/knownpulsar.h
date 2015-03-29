#ifndef KNOWNPULSAR
#define KNOWNPULSAR

struct KnownPulsar {
    int module;
    int ray;
    QTime time;
    double period;

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

