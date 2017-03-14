#include <reader.h>
#include <settings.h>

#include <malloc.h>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QApplication>

Reader::Reader(QObject *parent) :
    QObject(parent)
{
}

Data Reader::readFile(QString fileName, int skip, int firstColumn, QDateTime time, bool binary) {
    if (binary)
        return readBinaryFile(fileName);

    Data data;
    data.channels = 1;
    data.modules = 1;
    data.oneStep = 0.1;
    data.time = time;
    data.name = QFileInfo(fileName).fileName();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return data;

    QByteArray s;

    if (skip) {
        while (skip--)
            file.readLine();

        s = file.readLine();
    } else {
        while (!file.atEnd()) {
            s = file.readLine();
            if (s[0] >= '0' && s[0] <= '9')
                break;
        }
    }

    bool disableFirstRay = false;
    QList<QByteArray> res = s.split(' ');
    res.removeAll("");
    if (int(slowNumber(res[0])) == 1)
        disableFirstRay = true;

    if (firstColumn == 0)
        disableFirstRay = true;
    else if (firstColumn == 1)
        disableFirstRay = false;

    QVector<QString> input;
    while (s.size()) {
        input.push_back(s);
        s = file.readLine();
    }

    data.npoints = input.size();
    QStringList l = input[0].split(' ');
    l.removeAll("");
    data.rays = l.size() - disableFirstRay;
    data.init();

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
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "can't open file" << file << "for reading";
        Data sample;
        sample.npoints = 0;
        return sample;
    }

    int n = number(f.readLine());
    QMap<QString, QString> header;
    for (int i = 1; i < n; i++) {
        QString data = f.readLine();
        data.replace('\t', " ");
        QString value = data.right(data.size() - data.indexOf(' '));
        value = value.left(value.size() - 1);
        while (value[0] == ' ')
            value = value.right(value.size() - 1);

        header[data.left(data.indexOf(' '))] = value;
    }

    qint64 npoints = header["npoints"].toInt();
    int channels = header["nbands"].toInt() + 1;
    int rays = 8;
    QStringList t = header["modulus"].split(" ");
    t.removeAll("");
    int modulus = t.size();

    Data data;
    data.time.setDate(QDate::fromString(header["date_begin"].right(10), QString("dd.MM.yyyy")));
    data.time.setTime(QTime::fromString(header["time_begin"].right(8).replace(' ', ""), QString("h:mm:ss")));
    if (!header["time_begin"].contains("UTC"))
        data.time = data.time.addSecs(-3600 * 4);

    data.oneStep = header["tresolution"].toDouble() / 1000;
    data.delta_lucha = 0.89;
    data.name = QFileInfo(file).fileName();
    data.channels = channels;
    data.modules = modulus;
    data.rays = rays;
    data.npoints = npoints;
    data.init();

    data.previousLifeName = header["native_datetime"];
    if (data.previousLifeName == "")
        data.previousLifeName = file;

    QStringList fbands = header["fbands"].split(" ");
    for (int i = 0; i < channels - 1; i++)
        data.fbands[i] = fbands[i].toDouble();

    QByteArray input;
    input.resize(4 * 1024 * 1024);
    int remaining = 0;

    float *source;
    for (int i = 0; i < npoints; i++) {
        if (i % 1000 == 0)
            emit progress(i * 100 / npoints);

        qApp->processEvents();

        for (int m = 0; m < modulus; m++)
            for (int j = 0; j < rays; j++)
                for (int k = 0; k < channels; k++) {
                    if (!remaining) {
                        f.read(input.data(), 4 * 1024 * 1024);
                        remaining = 1024 * 1024;
                        source = (float*)(void*)input.data();
                    }

                    data.data[m][k][j][i] = (*source);
                    source++;
                    remaining--;
                }
        }

    emit progress(100);

    Settings::settings()->setLastHeader(header);
    Settings::settings()->setLastData(data);
    return data;
}

void Reader::repairWrongChannels(Data &data) {
    if (!data.name.right(15).contains("N2"))
        return;

    if (data.modules != 6 || data.rays != 8) {
        qDebug() << "Something strange with this data. Aborting" << data.name;
        return;
    }

    bool longData = (data.channels == 33);
    if (longData) {
        for (int module = 4; module < 6; module++)
            for (int ray = 6; ray >= 0; ray--)
                for (int channel = 23; channel < 32; channel++)
                    std::swap(data.data[module][channel][ray],
                              data.data[module][channel][7]);
    } else {
        for (int module = 4; module < 6; module++)
            for (int ray = 6; ray >= 0; ray--)
                for (int channel = 4; channel < 6; channel++)
                    std::swap(data.data[module][channel][ray],
                              data.data[module][channel][7]);
    }
}
