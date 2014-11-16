#include "pulsarreader.h"
#include <pulsar.h>

#include <QFile>
#include <QVariant>
#include <QList>
#include <QDateTime>
#include <QTextStream>

Pulsars PulsarReader::ReadPulsarFile(QString name) {
    QFile file(name);
    file.open(QIODevice::ReadOnly);
    file.readLine();
    file.readLine();

    Pulsars data = new QVector<Pulsar>;
    QVector<Pulsar> &res = *data;

    bool filtered = false;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line == "filtered next\n") {
            filtered = true;
        } else if (line == "additional data:\n") {
            line = file.readAll();
            QDataStream stream(&line, QIODevice::ReadOnly);

            for (int i = 0; i < res.size(); i++) {
                QVariant v;
                stream >> v;

                Data *data = &res[i].data;
                QVector<QVariant> vars = v.toList().toVector();

                res[i].noiseLevel = vars[vars.size() - 1].toDouble();

                data->channels = 1;
                data->delta_lucha = 0.89;
                data->modules = 1;
                data->npoints = vars.size() - 1;
                data->oneStep = res[i].period;
                data->rays = 1;
                data->time = QDateTime(QDate(2000, 1, 1), res[i].nativeTime);
                data->init();
                data->releaseProtected = true;
                data->sigma = res[i].noiseLevel;

                for (int i = 0; i < vars.size() - 1; i++)
                    data->data[0][0][0][i] = vars[i].toDouble();
            }
        } else {
            int h, m, s, module, ray, dispersion, filtered;
            double period, snr;
            QTextStream stream(line.data(), QIODevice::ReadOnly);
            char symb;
            stream >> h >> symb >> m >> symb >> s >> module >> ray >> dispersion >> period >> snr >> filtered;

            Pulsar pulsar;
            pulsar.dispersion = dispersion;
            pulsar.module = module;
            pulsar.ray = ray;
            pulsar.period = period;
            pulsar.snr = snr;
            pulsar.nativeTime = QTime(h, m, s);
            pulsar.filtered = filtered;

            res.push_back(pulsar);
        }
    }

    return data;
}
