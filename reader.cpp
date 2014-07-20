#include "reader.h"
#include <malloc.h>
#include <QFile>

Reader::Reader(QObject *parent) :
    QObject(parent)
{
}

Data Reader::readFile(QString fileName, int skip, int firstColumn, bool binary) {
    if (binary)
        return readBinaryFile(fileName);

    Data data;
    data.channels = 1;
    data.modules = 1;


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

    QVector<QString> input;
    while (s.size()) {
        input.push_back(s);
        s = file.readLine();
    }

    data.npoints = input.size();
    QStringList l = input[0].split(' ');
    l.removeAll("");
    data.rays = l.size() - disableFirstRay;
    data.data = new float***;
    data.data[0] = new float**;
    data.data[0][0] = new float*[data.rays];
    for (int i = 0; i < data.rays; i++)
        data.data[0][0][i] = new float[data.npoints];

    for (int i = 0; i < data.npoints; i++) {
        QStringList l = input[i].split(' ');
        l.removeAll("");
        for (int j = disableFirstRay; j < data.rays + disableFirstRay; j++)
            data.data[0][0][j - disableFirstRay][i] = slowNumber(l[j].toUtf8());

        if (i % 1000 == 0)
            emit progress(i * 100 / data.npoints);
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

float Reader::slowNumber(QByteArray a) {
    if (a[a.size() - 1] == '\n')
        a.resize(a.size() - 1);

    return QString(a).toFloat();
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
    data.channels = channels + 1;
    data.modules = modulus;
    data.rays = rays;
    data.npoints = npoints;
    data.data = new float***[modulus];
    for (int j = 0; j < modulus; j++) {
        data.data[j] = new float**[channels + 1];
        for (int i = 0; i < channels + 1; i++) {
            data.data[j][i] = new float*[rays];
            for (int k = 0; k < rays; k++)
                data.data[j][i][k] = new float[npoints];
        }
    }



    QByteArray input;
    input.resize(4 * 1024 * 1024);
    int remaining = 0;

    float *source;
    for (int i = 0; i < npoints; i++) {
        if (i % 1000 == 0)
            emit progress(i * 100 / npoints);

        for (int m = 0; m < modulus; m++)
            for (int j = 0; j < rays; j++)
                for (int k = 0; k < channels + 1; k++) {
                    if (!remaining) {
                        f.read(input.data(), 4 * 1024 * 1024);
                        remaining = 1024 * 1024;
                        source = (float*)(void*)input.data();
                    }

                    data.data[m][k][j][i] = (*source) * 1000000;
                    source++;
                    remaining--;
                }
        }

    return data;
}
