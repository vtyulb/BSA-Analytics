#include "spectredrawer.h"

#include <QVector>
#include <QImage>
#include <QDir>
#include <QDesktopServices>
#include <QPainter>
#include <QUrl>

#include <reader.h>
#include <pulsarworker.h>
#include <startime.h>

#include <algorithm>

using std::min;

QVector<int> SpectreDrawer::getAnswer(const Data &data, int channel, int module, int ray, QTime time, double period) {
    int start = 0;
    while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(data, start)))) > interval / 2)
        start++;

    QVector<double> res;
    //hello pulsar.h::calculateAdditionalData
    for (int offset = -period / data.oneStep / 2; offset < period / data.oneStep * 2 - period / data.oneStep / 2 + 1; offset++) {
        double sum = 0;
        int n = 0;
        for (double i = start + offset; i < start + offset  + interval / data.oneStep; i += period / data.oneStep * 2, n++)
            sum += data.data[module][channel][ray][int(i)];

        res.push_back(sum / n);
    }

    double max = 0;
    for (int i = 0; i < res.size(); i++)
        if (max < res[i])
            max = res[i];

    QVector<int> norm;
    for (int i = 0; i < res.size(); i++)
        norm.push_back(255 * res[i] / max);

    return norm;
}

void SpectreDrawer::drawSpectre(int module, int ray, QString fileName, QTime time, double period) {
    Reader r;
    Data data = r.readBinaryFile(fileName);

    const int step =  INTERVAL / data.oneStep;
    for (int channel = 0; channel < data.channels - 1; channel++)
        for (int i = 0; i < data.npoints; i += step)
            PulsarWorker::subtract(data.data[module][channel][ray] + i, min(step, data.npoints - i));

    QVector<QVector<int> > matrix;
    matrix.resize(data.channels - 1);
    for (int i = 0; i < matrix.size(); i++)
        matrix[i] = getAnswer(data, i, module, ray, time, period);

    drawImage(matrix);
}

void SpectreDrawer::drawImage(QVector<QVector<int> > matrix) {
    QImage image(matrix[0].size(), matrix.size(), QImage::Format_ARGB32);
    QPainter p(&image);

    for (int i = 0; i < matrix.size(); i++)
        for (int j = 0; j < matrix[i].size(); j++) {
            int color = matrix[i][j];
            p.setPen(QColor(color, color, color));
            p.drawPoint(j, i);
        }


    p.end();
    image.save(QDir::tempPath() + "/spectre.png", "png");
    QDesktopServices::openUrl(QUrl(QDir::tempPath() + "/spectre.png"));
}
