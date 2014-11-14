#include "pulsarreader.h"

#include <QFile>
#include <QVariant>
#include <QList>
#include <QDateTime>

QVector<Pulsar> PulsarReader::ReadPulsarFile(QString name) {
    QFile file(name);
    file.open(QIODevice::ReadOnly);
    file.readLine();
    file.readLine();

    QVector<Pulsar> res;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line == "additional data:\n") {
            line = file.readLine();
            QList<QByteArray> list = line.split(':');
            list.removeAll(QByteArray());
            QVector<QByteArray> r = list.toVector();
            for (int i = 0; i < r.size(); i++) {
                r[i] = QByteArray::fromBase64(r[i]);
                QVariant v;
                QDataStream stream(&r[i], QIODevice::ReadOnly);
                v.load(stream);


                Data *data = &res[i].data;
                QVector<QVariant> vars = v.toList().toVector();

                data->channels = 1;
                data->delta_lucha = 0.89;
                data->modules = 1;
                data->npoints = vars.size() - 1;
                data->oneStep = res[i].period;
                data->rays = 1;
                data->time = QDateTime();
                data->init();

                res[i].noiseLevel = vars[vars.size() - 1].toDouble();

                for (int i = 0; i < vars.size() - 1; i++)
                    data->data[0][0][0][i] = vars[i].toDouble();
            }
        } else {
            int h, m, s, module, ray, dispersion;
            double period, snr;
            sscanf(line.constData(), "%d:%d:%d %d %d %d %lf %lf", &h, &m, &s, &module, &ray, &dispersion, &period, &snr);

            Pulsar pulsar;
            pulsar.dispersion = dispersion;
            pulsar.module = module;
            pulsar.ray = ray;
            pulsar.period = period;
            pulsar.snr = snr;
            pulsar.nativeTime = QTime(h, m, s);

            res.push_back(pulsar);
        }
    }

    return res;
}
