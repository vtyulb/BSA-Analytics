#include "transientdetalizator.h"

#include <data.h>
#include <drawer.h>
#include <reader.h>
#include <settings.h>
#include <startime.h>

#include <QTime>
#include <QMessageBox>
#include <QString>

void TransientDetalizator::run(int module, int ray, QTime time, QString file, Data data) {
    module--;
    ray--;
    Reader reader;
    QObject::connect(&reader, SIGNAL(progress(int)), Settings::settings(), SLOT(setProgress(int)));

    Data source = data;
    if (!source.isValid())
        source = reader.readBinaryFile(file);

    if (!source.isValid()) {
        QMessageBox::warning(NULL, "Problem with source", "Can't read file!");
        return;
    }

    if (source.npoints <= 500)
        source.channels -= 1;

    Data res = source;
    res.modules = 1;
    res.rays = source.channels;
    res.channels = 1;
    res.npoints = 500;
    res.init();

    int point = 500;

    if (source.npoints > 500) {
        while (abs(time.secsTo(QTime::fromString(StarTime::StarTime(source, point)))) > 1)
            point++;

        point -= res.npoints / 2;
    } else {
        point = 0;
        res.npoints = source.npoints;
        res.releaseData();
        res.init();
    }

    res.time = res.time.addMSecs(12.4928 * point);

    for (int i = 0; i < source.channels; i++)
        memcpy(res.data[0][0][i],
               source.data[module][i][ray] + point,
               sizeof(float) * res.npoints);

    if (source.npoints > 500)
        for (int i = 0; i < source.channels / 2; i++)
            std::swap(res.data[0][0][i], res.data[0][0][res.rays - 1 - i]);

    source.releaseData();

    double level = 1.0;
    double av = 0;
    for (int i = 0; i < source.channels; i++) {
        QVector<float> cur;
        for (int j = 0; j < res.npoints; j++)
            cur.push_back(res.data[0][0][i][j]);

        std::sort(cur.begin(), cur.end());
        av += cur.at(cur.size() / 2) / source.channels;
    }

    for (int i = source.channels - 1; i >= 0; i--) {
        QVector<float> cur;
        for (int j = 0; j < res.npoints; j++)
            cur.push_back(res.data[0][0][i][j]);

        std::sort(cur.begin(), cur.end());
        level += cur.at(cur.size() * 0.8) - cur.at(cur.size() * 0.2) * 2;
        for (int j = 0; j < res.npoints; j++)
            res.data[0][0][i][j] -= av - level;
    }

    Drawer *drawer = new Drawer(res);
    drawer->show();
    drawer->showMaximized();
}
