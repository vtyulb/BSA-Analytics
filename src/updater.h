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

  private slots:
    void runSetup(QNetworkReply*);
    void downloadProgressChanged(qint64, qint64);

  public slots:
    void download();
    void cancelUpdate();
};

#endif // UPDATER_H
