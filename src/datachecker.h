#ifndef DATACHECKER_H
#define DATACHECKER_H

#include <QDate>
#include <QString>
#include <QStringList>

class DataChecker
{
public:
    DataChecker() {};

    void run(QString);
    void search(QString);

private:
    QStringList files;
    QString globalFilter;

    void processWithFilter(QString filter);
    QDateTime stringToDate(QString);
    QString dateToString(QDateTime, QString localFilter);
};

#endif // DATACHECKER_H
