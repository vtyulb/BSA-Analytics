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
    data[0].resize(1);
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

        data[0][0].push_back(layer);

        s = file.readLine();
        if (s[0] == '@')
            break;
        readed += s.size();

        if (line % 10000 == 0)
            emit progress(100*readed/total);
    }

    emit progress(100);
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

    qint64 npoints = header["npoints"].toInt();
    int channels = header["nbands"].toInt();
    int rays = 8;
    int modulus = 6;

    Data data;
    data.resize(modulus);
    for (int j = 0; j < modulus; j++) {
        data[j].resize(channels + 1);
        for (int i = 0; i < channels + 1; i++) {
            data[j][i].resize(npoints);
            for (int k = 0; k < npoints; k++)
                data[j][i][k].resize(rays);
        }
    }

    QByteArray a(npoints * modulus * rays * (channels + 1) * 4, ' ');
    f.read(a.data(), npoints * modulus * rays * (channels + 1) * 4);

    float *source = (float*)(void*)a.data();
    for (int i = 0; i < npoints; i++) {
        if (i % 1000 == 0)
            emit progress(((char*)source - a.data()) * 100 / a.size());

        for (int m = 0; m < modulus; m++)
            for (int j = 0; j < rays; j++)
                for (int k = 0; k < channels + 1; k++) {
                    data[m][k][i][j] = *source * 1000000;
                    source++;
                }
        }

    return data;
}
