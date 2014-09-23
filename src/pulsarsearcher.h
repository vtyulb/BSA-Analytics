#ifndef PULSARSEARCHER_H
#define PULSARSEARCHER_H

#include <QObject>
#include <QFileInfoList>

class PulsarSearcher : public QObject
{
    Q_OBJECT
public:
    explicit PulsarSearcher(QString dir, QObject *parent = 0);
    void start();

private:
    QFileInfoList files;

signals:

public slots:

};

#endif // PULSARSEARCHER_H
