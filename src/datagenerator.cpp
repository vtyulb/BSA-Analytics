#include "datagenerator.h"

#include <QImage>
#include <QPainter>
#include <QPair>
#include <QVector>

Data DataGenerator::generateRandomPhrase() {
    QStringList phrases;
    phrases << "Hello!";
    phrases << "BSA";
    phrases << "BSA-Analytics by vtyulb";
    phrases << "serg@prao.ru";
    phrases << "vtyulb@vtyulb.ru";
    phrases << "Help me, I am locked in BSA-Analytics engine!";
    phrases << "Bugtracker is located at https://github.com/vtyulb/BSA-Analytics/issues";

    return generate(phrases[rand() % phrases.size()]);
}

Data DataGenerator::generate(QString phrase) {
    QFont font("Liberation Mono", 14);
    font.setStyleStrategy(QFont::NoAntialias);

#ifdef Q_OS_LINUX
    font = QFont("", 14);
#endif

    const int width = QFontMetrics(font).width(phrase) + 5;
    const int height = 100;
    const bool longPhrase = phrase.size() > 25;

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(QColor("white"));
    QPainter p(&image);
    p.setFont(font);
    p.drawText(1, 50, phrase);
    p.end();

    const int RC = 8;
    const int defaultHeight = 70 + phrase.size() * 2;
    const QPair<int, int> def(defaultHeight, defaultHeight);

    QVector<double> raysData[RC];
    QPair<int, int> rays[RC];
    for (int i = 0; i < RC; i++)
        rays[i] = def;

    for (int i = 0; i < width; i++) {
        for (int j = 1; j < RC; j++) {
            int a = rand() % (j + 1);
            std::swap(rays[a], rays[j]);
            raysData[j].swap(raysData[a]);
        }

        QVector<QPair<int, int>> domains;
        for (int j = 0; j < height; j++)
            if (image.pixel(i, j) != QColor("white").rgb()) {
                int start = j;
                while (image.pixel(i, j) != QColor("white").rgb())
                    j++;

                domains.push_back(QPair<int, int>(start, j - 1));
            }

        domains.push_back(def);
        QVector<bool> covered(domains.size(), false);
        QVector<bool> raySet(RC, false);
        // setting obvious rays ------------------------------
        for (int j = 0; j < RC; j++) {
            for (int k = 0; k < domains.size(); k++)
                if (rays[j].first <= domains[k].second + 1 && rays[j].second + 1 >= domains[k].first && !covered[k]) {
                    covered[k] = true;
                    raySet[j] = true;
                    rays[j] = domains[k];
                    break;
                }
        }

        // setting uncovered domains -------------------------
        for (int j = 0; j < domains.size(); j++)
            if (!covered[j]) {
                for (int i = 0; i < RC; i++)
                    if (!raySet[i]) {
                        covered[j] = true;
                        raySet[i] = true;
                        rays[i] = domains[j];
                        break;
                    }
            }

        // setting other rays
        for (int j = 0; j < RC; j++)
            if (!raySet[j]) {
                for (int k = 0; k < domains.size() - 1; k++)
                    if (rays[j].first <= domains[k].second + longPhrase && longPhrase + rays[j].second >= domains[k].first) {
                        rays[j] = domains[k];
                        raySet[j] = true;
                        break;
                    }

                if (!raySet[j])
                    rays[j] = domains[rand() % domains.size()];
            }

        // setting raw data ----------------------------------
        for (int j = 0; j < RC; j++)
            fillRay(raysData[j], rays[j]);
    }

    Data res;
    res.modules = 1;
    res.channels = 1;
    res.rays = RC;
    res.npoints = raysData[0].size();
    res.init();
    for (int i = 0; i < RC; i++)
        for (int j = 0; j < raysData[i].size(); j++)
            res.data[0][0][i][j] = raysData[i][j];

    return res;
}

void DataGenerator::fillRay(QVector<double> &ray, QPair<int, int> range) {
    for (int i = 0; i < 50; i++) {
        int k = rand() % 100;
        ray.push_back(100 - range.first - k * (range.second - range.first + 1) / 100.0);
    }
}
