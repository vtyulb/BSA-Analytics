#include "pulsarreader.h"
#include <pulsar.h>
#include <settings.h>

#include <QFile>
#include <QVariant>
#include <QList>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QApplication>

Pulsars PulsarReader::ReadPulsarFile(QString name, QProgressBar *bar) {
    if (!name.endsWith(".pulsar")) {
        qDebug() << "wrong file name" << name;
        return new QVector<Pulsar>;
    }

    qDebug() << "reading file" << name;

    QFile file(name);
    file.open(QIODevice::ReadOnly);
    QString fileName = file.readLine();
    fileName = fileName.right(fileName.size() - 6);
    fileName = fileName.left(fileName.size() - 1);
    QString oneStep = file.readLine();
    oneStep = oneStep.right(oneStep.size() - 12);
    oneStep.chop(1);
    Settings::settings()->setRealOneStep(oneStep.toDouble());
    file.readLine();

    Pulsars data = new QVector<Pulsar>;
    QVector<Pulsar> &res = *data;

    bool filtered = false;
    bar->show();

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line == "filtered next\n") {
            filtered = true;
        } else if (line == "additional data:\n") {
            QDataStream stream(&file);

            for (int i = 0; i < res.size(); i++) {
                if (bar && i % 100 == 0) {
                    bar->setValue(file.pos() * 100 / file.size());
                    qApp->processEvents();
                }

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
            int h = 23, m = 59, s = 59, module, ray, dispersion;
            double period, snr;
            int firstPoint;
            QTextStream stream(line.data(), QIODevice::ReadOnly);
            char symb;
            if (line.data()[0] >= '0' && line.data()[0] <= '9')
                stream >> h >> symb >> m >> symb >> s;
            stream >> module >> ray >> dispersion >> period >> snr >> firstPoint;

            Pulsar pulsar;
            pulsar.dispersion = dispersion;
            pulsar.module = module;
            pulsar.ray = ray;
            pulsar.period = period;
            pulsar.snr = snr;
            pulsar.nativeTime = QTime(h, m, s);
            pulsar.filtered = filtered;
            pulsar.firstPoint = firstPoint;
            pulsar.data.name = fileName;

            int badNoise;
            stream >> badNoise;
            if (!stream.atEnd()) {
                pulsar.badNoiseKnown = true;
                pulsar.badNoiseRes = badNoise;

                stream >> pulsar.data.name;
            }

            res.push_back(pulsar);
        }
    }

    bar->hide();

    return data;
}
