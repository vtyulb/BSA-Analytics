#include "reader.h"
#include <QFile>

Reader::Reader(QObject *parent) :
    QObject(parent)
{
}

Data Reader::readFile(QString fileName, int skip, int firstColumn) {
    Data data;
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
        QVector<int> layer;
        for (int i = 0; i < res.size(); i++)
            if (i != 0 || !disableFirstRay)
                layer.push_back(number(res[i]));

        data.push_back(layer);

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

int Reader::number(QByteArray &a) {
    if (a[a.size() - 1] == '\n')
        a.resize(a.size() - 1);

    return QString(a).toInt();
}
