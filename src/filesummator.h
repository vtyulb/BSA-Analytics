#ifndef FILESUMMATOR_H
#define FILESUMMATOR_H

#include <QStringList>
#include <QVector>

#include <data.h>

class FileSummator
{
public:
    FileSummator();

    void run();

private:
    void findFiles(QString path, QStringList &names, const QStringList &extensions);
    void processData(Data &data);

    QVector<float> sigmas;
    float goodSigma;
};

#endif // FILESUMMATOR_H
