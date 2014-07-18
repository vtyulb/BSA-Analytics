#include "reader.h"
#include <QFile>

Reader::Reader(QObject *parent) :
    QObject(parent)
{
}

Data Reader::readFile(QString fileName, int skip, int firstColumn, bool binary) {
    if (binary)
        return readBinaryFile(fileName);

    Data data;
    data.resize(1);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return data;

    while (skip--)
        file.readLine();

    QByteArray s;
    while (!file.atEnd()) {
        s = file.readLine();
        if (s[0] >= '0' && s[0] <= '9')
            break;
    }

    bool disableFirstRay = false;
    QList<QByteArray> res = s.split(' ');
    if (number(res[0]) == 1)
        disableFirstRay = true;

    if (firstColumn == 0)
        disableFirstRay = true;
    else if (firstColumn == 1)
        disableFirstRay = false;

    double readed = 0;
    double total = file.size();
    int line = 0;

    while (s.size()) {
        line++;
        QList<QByteArray> res = s.split(' ');
        QVector<float> layer;
        for (int i = 0; i < res.size(); i++)
            if (i != 0 || !disableFirstRay)
                layer.push_back(number(res[i]));

        data[0].push_back(layer);

        s = file.readLine();
        if (s[0] == '@')
            break;
        readed += s.size();

        if (line % 10000 == 0)
            emit progress(100*readed/total);
    }

    emit progress(100);

    qDebug() << "readed" << data.size() << "lines with" << data[0].size() << "rays";

    return data;
}

int Reader::number(QByteArray a) {
    if (a[a.size() - 1] == '\n')
        a.resize(a.size() - 1);

    int i;
    for (i = a.size() - 1; i; i--)
        if (a[i] == ' ')
            break;

    return QString(a.right(a.size() - i)).toInt();
}

Data Reader::readBinaryFile(QString file) {
    QFile f(file);
    f.open(QIODevice::ReadOnly);

    int n = number(f.readLine());
    QMap<QString, QString> header;
    for (int i = 1; i < n; i++) {
        QString data = f.readLine();
        header[data.left(data.indexOf(' '))] = data.right(data.size() - data.indexOf(' '));
    }

    int npoints = header["npoints"].toInt();
    int channels = header["nbands"].toInt();
    int rays = 8;

    Data data;
    data.resize(channels + 1);
    for (int i = 0; i < channels + 1; i++)
        data[i].resize(npoints);

    QByteArray a = f.readAll();
    char *source = a.data();
    for (int i = 0; i < npoints; i++) {
        if (i % 1000 == 0)
            emit progress((source - a.data()) * 400 / a.size());

        for (int j = 0; j < rays; j++)
            for (int k = 0; k < channels + 1; k++) {
                data[k][i].push_back(decode(source));
                source += 4;
            }
    }

    return data;
}

float Reader::decode(char *c) {
    float res = *((float *)(void *)c);
    return res * 1000000;
}
