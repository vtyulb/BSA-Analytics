#ifndef FILEQSUMMATOR_H
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
    void processData(Data &data, Data &multifile, Data &coefficients);

    QVector<float> sigmas;
    float goodSigma;
    int stage;

    QVector<QVector<float> > noises;
};

#endif // FILESUMMATOR_H