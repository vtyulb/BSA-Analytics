#ifndef MASSPROCESSOR_H
#define MASSPROCESSOR_H

#include <QStringList>
#include <QVector>
#include <QSet>

#include <data.h>

class MassProcessor
{
public:
    MassProcessor();

    void runInteractive();
    void runFluxDensity(QString path, int module, int ray, QTime time);

private:
    void findFiles(QString path, QStringList &names, const QStringList &extensions);
    void transientProcess(Data &data);
    bool processData(Data &data);
    bool processLongData(Data &data);
    void processFRB(Data &data, int module, int ray, int offset, QVector<double> &autoCorrelation, int block);
    void dumpCuttedPiece(const Data &data, int startPoint, int pieceNumber);
    bool dumpTransient(const QVector<double> &data, const Data &rawData, int startPoint, int pieceNumber, int module, int ray, int dispersion, double snr);
    bool transientCheckAmplification(const Data &data, int point, int module, int ray, int dispersion);

    void addStair(Data &stairs);
    bool findStair(Data &data, int &start, int &end);
    void checkStairs(Data &stairs, QStringList &names);
    void sortStairs(Data &stairs, QStringList &names);
    void dumpStairs(const Data &stairs, const QStringList &names);
    void initStairs(Data &stairs, QStringList &names);
    QString getStairsName(const Data &data);

    void fluxCheck(Data &flux, QStringList &names);
    QVector<double> sourceAutoDetect(Data &data, int module, int ray, QTime time);
    float median(float *data, int element);


    void saveCuttingState();
    void loadCuttingState();

    QVector<double> applyDispersion(Data &data, int D, int module, int ray);

    bool longData = false;
    bool FRBmode = false;
    bool basicGraphicFilterEnabled = false;
    int PC;
    int firstDispersion, lastDispersion;
    QString cutterPath;

    Data stairs;
    QStringList stairsNames;
    QString stairsNameOverride;
    QVector<QVector<QVector<float> > > noises;

    QVector<int> numberOfPieces;
    QSet<QString> filesProcessed;
};

#endif // MASSPROCESSOR_H
