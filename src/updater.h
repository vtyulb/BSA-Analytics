#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QNetworkReply>
#include <QProgressBar>

class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(QObject *parent = 0);

private:
    QProgressBar *progress;
    QWidget *downloaderWidget;
    QNetworkReply *networkReply;
    qint64 latestInstallerSize;
    QNetworkAccessManager *manager;

private slots:
    void dumpSetup(QNetworkReply*);
    void runSetup();
    void downloadProgressChanged(qint64, qint64);
    void cancelUpdate();
    void download();
    void checkFinished(QNetworkReply*);

public slots:
    void checkForUpdates();
};

#endif // UPDATER_H
