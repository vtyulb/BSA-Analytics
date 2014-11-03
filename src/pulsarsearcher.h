#ifndef PULSARSEARCHER_H
#define PULSARSEARCHER_H

#include <QObject>
#include <QFileInfoList>
#include <QVector>
#include <pulsarprocess.h>

class PulsarSearcher : public QObject
{
    Q_OBJECT
public:
    explicit PulsarSearcher(QString dir, QObject *parent = 0);
    void start();

private:
    QFileInfoList files;
    QVector<PulsarProcess*> workers;

private slots:
    void checkIfCalculated();

};

#endif // PULSARSEARCHER_H
