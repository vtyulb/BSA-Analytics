#include "datachecker.h"

#include <QDebug>
#include <QDir>

void DataChecker::run(QString path) {
    search(QDir(path).absolutePath());


    if (files.size() == 0) {
        printf("No files found, please check the path: %s\n", path.toLocal8Bit().constData());
        return;
    }

    globalFilter = files.first().endsWith("pnt") ? "pnt" : "pnthr";
    for (int i = 0; i < files.size(); i++)
        if (!files.at(i).endsWith(globalFilter)) {
            files.removeAt(i);
            i--;
        }

    processWithFilter("_N1_");
    processWithFilter("_N2_");
    checkSizes();
}

void DataChecker::checkSizes() {
    for (int i = 0; i < fullNames.size(); i++) {
        long long size = QFileInfo(fullNames[i]).size();
        if (size < 1700 * 1000 * 1000 && globalFilter == "pnthr")
            printf("File %s has invalid size %dM\n", fullNames[i].toLocal8Bit().constData(), (int)size / 1000000);
    }
}

void DataChecker::processWithFilter(QString filter) {
    QVector<QDateTime> lst;
    for (int i = 0; i < files.size(); i++)
        if (files[i].contains(filter))
            lst.push_back(stringToDate(files[i]));

    if (lst.size() == 0) {
        printf("No data found for filter %s\n", filter.toLocal8Bit().constData());
        return;
    }

    std::sort(lst.begin(), lst.end());
    printf("Filter: %s\n", filter.toLocal8Bit().constData());
    printf("First file: %s\n", dateToString(lst.first(), filter).toLocal8Bit().constData());
    printf("Last file: %s\n", dateToString(lst.last(), filter).toLocal8Bit().constData());
    for (int i = 1; i < lst.size(); i++)
        if (lst[i - 1].secsTo(lst[i]) > 5000) {
            QDateTime mBegin = lst[i - 1].addSecs(3600);
            QDateTime mEnd = lst[i].addSecs(-3600);
            printf("Missing interval: %s - %s. Lost %lld hours.\n", dateToString(mBegin, filter).toLocal8Bit().constData(),
                                                                  dateToString(mEnd, filter).toLocal8Bit().constData(),
                                                                  mBegin.secsTo(mEnd) / 3600 + 1);
        }

    printf("------------\n");
}

void DataChecker::search(QString path) {
    QDir dir(path);
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); i++)
        search(path + "/" + dirs.at(i));

    QStringList fls = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; i < fls.size(); i++)
        fullNames += path + "/" + fls[i];

    files += fls;
}

QDateTime DataChecker::stringToDate(QString date) {
    return QDateTime(QDate(2000 + date.left(6).right(2).toInt(),
                                  date.left(4).right(2).toInt(),
                                  date.left(2).right(2).toInt()),
                     QTime(date.left(9).right(2).toInt(), 0));
}

QString DataChecker::dateToString(QDateTime date, QString localFilter) {
    return QString::asprintf("%02d%02d%s_%02d%s00.%s",
                             date.date().day(),
                             date.date().month(),
                             QString::number(date.date().year() % 100).toLocal8Bit().constData(),
                             date.time().hour(),
                             localFilter.toLocal8Bit().constData(),
                             globalFilter.toLocal8Bit().constData());
}
