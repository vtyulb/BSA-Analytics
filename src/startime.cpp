#include "startime.h"

#include <math.h>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.141592653589793238
#endif

namespace {
    double sqr(double a) {
        return a * a;
    }

    QPair<QString, QString> precess(double alfa, double delt, double t, double *realSeconds){
        double rs = 4.8481368e-6;
        double am = 46.1 * rs;
        double an = 20.4 * rs;

        double alfa1 = alfa;
        double delta1 = delt;

        double alf;
        double del;

        for (int i = 0; i < 2; i++) {
            alf = alfa - (am + an * sin(alfa1) * tan(delta1)) * t;
            del = delt - an * cos(alfa1) * t;
            alfa1 = (alf + alfa) / 2;
            delta1 = (del + delt) / 2;
        }

        alf /= (15 * rs);
        del /= rs;

        int m[4];
        int n[4]; // do not touch this constants

        alf /= 3600;
        while (alf >= 24)
            alf -= 24;

        while (alf < 0)
            alf += 24;

        m[1] = alf;
        m[2] = (alf - m[1]) * 60;
        m[3] = ((alf - m[1]) * 60 - m[2]) * 60;

        if (realSeconds) {
            *realSeconds = alf * 3600;
        }

        m[2] += m[3] / 60;
        m[3] %= 60;
        m[1] += m[2] / 60;
        m[2] %= 60;
        m[1] %= 24;

        del /= 3600;
        while (del >= 360)
            del -= 360;

        while (del < 0)
            del += 360;


        n[1] = del;
        n[2] = (del - n[1]) * 60;
        n[3] = ((del - n[1]) * 60 - n[2]) * 60;

        n[2] += n[3] / 60;
        n[3] %= 60;
        n[1] += n[2] / 60;
        n[2] %= 60;
        n[1] %= 24;


        QPair<QString, QString> res = QPair<QString, QString>(QTime(m[1], m[2], m[3]).toString("hh:mm:ss"), QString("%1°%2.%3").arg(n[1]).arg(n[2]).arg(n[3]));
        return res;
    }

    QPair<QString, QString> get_alf(double culm, double delt, double *realSeconds) {
        const double fi = 0.956829;
        const double be = 0.008436;

        double aa = sqr(sin(fi)) * sqr(cos(be)) + sqr(cos(fi));
        double bb = 2 * sin(fi) * cos(be) * sin(delt);
        double cc = sqr(sin(delt)) - sqr(cos(fi));

        double x = (bb + sqrt(sqr(bb) - 4 * aa * cc)) / (2 * aa);
        double y = x * sin(be) / cos(delt);
        double z = sqrt(1 - y * y);

        double t = y / z;
        double alf = culm * M_PI / 12 + atan(t);

        return precess(alf, delt, t, realSeconds);
    }
}

namespace StarTime {
    QString StarTime(Data data, int point, double *realSeconds) {
        if (!data.time.isValid()) {
//            qDebug() << "invalid time";
            return "";
        }

        QDateTime time = data.time.addMSecs(data.oneStep * point * 1000);
        double delta_lucha = data.delta_lucha;

        double t = (time.date().toJulianDay() - QDate(2000, 1, 1).toJulianDay() - 1);
        t /= 36525;

        double s0 = 6 + 41 / 60.0 + 50.55 / 3600.0 + 8640184 / 3600.0 * t + 0.093104 / 3600.0 * t * t - 6.27 / 3600.0 * (1e-6) * t * t * t;
        double t_culm = time.time().hour() + time.time().minute() / 60.0 + time.time().second() / 3600.0 + time.time().msec() / 3600.0 / 1000.0;
        double alambda = 2 + 30/60.0 + 34/3600.0;
        double cnst = 2.7379093e-3;
        double s_culm = s0 + (cnst + 1) * t_culm + alambda;

        QPair<QString, QString> res = get_alf(s_culm, delta_lucha, realSeconds);
        if (point == -1)
            return res.second;
        else
            return res.first;
    }
}
