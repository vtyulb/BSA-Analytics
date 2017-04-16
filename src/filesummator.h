#ifndef FILEQSUMMATOR_H
#define FILESUMMATOR_H

#include <QStringList>
#include <QVector>
#include <QSet>

#include <data.h>

class FileSummator
{
public:
    FileSummator();

    void run();

private:
    void findFiles(QString path, QStringList &names, const QStringList &extensions);
    void transientProcess(Data &data);
    bool processData(Data &data);
    void processLongData(Data &data);
    void dumpCuttedPiece(const Data &data, int startPoint, int pieceNumber);
    void dumpTransient(const QVector<double> &data, const Data &rawData, int startPoint, int pieceNumber, int module, int ray, int dispersion);
    bool transientCheckAmplification(const Data &data, int point, int module, int ray, int dispersion);

    void addStair(Data &stairs);
    bool findStair(Data &data, int &start, int &end);
    void checkStairs(Data &stairs, QStringList &names);
    void sortStairs(Data &stairs, QStringList &names);
    void dumpStairs(const Data &stairs, const QStringList &names);
    void initStairs(Data &stairs, QStringList &names);
    QString getStairsName(const Data &data);

    void saveCuttingState();
    void loadCuttingState();

    QVector<double> applyDispersion(Data &data, int D, int module, int ray);

    bool longData = false;
    int PC;
    QString cutterPath;

    Data stairs;
    QStringList stairsNames;
    QString stairsNameOverride;
    QVector<QVector<QVector<float> > > noises;

    QVector<int> numberOfPieces;
    QSet<QString> filesProcessed;
};

#endif // FILESUMMATOR_H
