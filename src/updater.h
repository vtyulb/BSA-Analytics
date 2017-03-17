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
    QNetworkAccessManager *manager;
    bool silentMode;

    long long getNativeRAMcount();
    QByteArray getRAMcount();
    QByteArray getSystemInfo();

private slots:
    void dumpSetup(QNetworkReply*);
    void runSetup();
    void downloadProgressChanged(qint64, qint64);
    void cancelUpdate();
    void download();
    void checkFinished(QNetworkReply*);

public slots:
    void checkForUpdates(bool silence);
};

#endif // UPDATER_H
