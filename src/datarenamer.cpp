#include "datarenamer.h"

#include <QDir>

void DataRenamer::run(QString path) {
    if (!QDir(path).exists()) {
        printf("Path \"%s\" not found\n", path.toLocal8Bit().constData());
        return;
    }

    QString base = QDir(path).absolutePath() + "/";

    for (int i = 0; i < 500; i++)
        repairDir(base + QString::asprintf("%03d/", i));
}

void DataRenamer::repairDir(QString path) {
    QStringList names = QDir(path).entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; i < names.size(); i++) {
        if (rand() % 100 == 0)
            printf("%s\n", (path + names[i]).toLocal8Bit().constData());
        QFile f(path + names[i]);
        f.rename(path + QString::asprintf("%06d.pnt", i));
    }
}
