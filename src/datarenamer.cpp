#include "datarenamer.h"

#include <QDir>

void DataRenamer::run(QString path) {
    QStringList paths = path.split(",");
    for (int i = 0; i < paths.size(); i++) {
        QString path = paths[i];
        if (!QDir(path).exists()) {
            printf("Path \"%s\" not found\n", path.toLocal8Bit().constData());
            return;
        }
    }

    for (int i = 0; i < paths.size(); i++)
        paths[i] = QDir(paths[i]).absolutePath() + "/";

    for (int i = 0; i < 500; i++)
        repairDir(paths, i);
//        repairDir(base + QString::asprintf("%03d/", i));
}

void DataRenamer::repairDir(QStringList paths, int block) {
    QStringList names;
    for (int i = 0; i < paths.size(); i++) {
        QStringList tmp = QDir(paths[i] + QString::asprintf("%03d/", block)).entryList(QDir::Files | QDir::NoDotAndDotDot);
        for (int j = 0; j < tmp.size(); j++)
            names.push_back(paths[i] + QString::asprintf("%03d/", block) + tmp[j]);
    }

    for (int i = 0; i < names.size(); i++) {
        if (rand() % 100 == 0)
            printf("%s\n", (names[i]).toLocal8Bit().constData());

        QDir().mkpath(paths.first() + QString::asprintf("%03d/", block));
        QString newName = paths.first() + QString::asprintf("%03d/%06d.pnt", block, i);
        QDir().rename(names[i], newName);
    }
}
