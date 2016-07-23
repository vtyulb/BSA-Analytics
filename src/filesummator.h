#ifndef FILESUMMATOR_H
#define FILESUMMATOR_H

#include <QStringList>

#include <data.h>

class FileSummator
{
public:
    FileSummator();

    void run();

private:
    void findFiles(QString path, QStringList &names, const QStringList &extensions);
    void processData(Data &data);
};

#endif // FILESUMMATOR_H
