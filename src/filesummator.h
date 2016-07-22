#ifndef FILESUMMATOR_H
#define FILESUMMATOR_H

#include <QStringList>

class FileSummator
{
public:
    FileSummator();

    void run();

private:
    void findFiles(QString path, QStringList &names, const QStringList &extensions);
};

#endif // FILESUMMATOR_H
