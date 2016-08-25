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
    void processLongData(Data &data);
    void dumpCuttedPiece(const Data &data, int startPoint, int pieceNumber);

    int stage;
    bool longData = false;
    int PC;
    QString cutterPath;

    QVector<QVector<float> > noises;
    QVector<int> numberOfPieces;
};

#endif // FILESUMMATOR_H
