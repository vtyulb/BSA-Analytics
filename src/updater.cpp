#include "updater.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QDesktopServices>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>

const QString installerName = QDir::tempPath() + "/BSA-Analytics-x64.exe";

Updater::Updater(QObject *parent) : QObject(parent)
{
}

void Updater::download() {
    QNetworkAccessManager *manager = new QNetworkAccessManager;
    QUrl url("https://bsa.vtyulb.ru/BSA-Analytics-x64.exe");
    QNetworkRequest request(url);

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(runSetup(QNetworkReply*)));

    downloaderWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(downloaderWidget);
    layout->addWidget(new QLabel("Downloading latest version of BSA-Analytics-x64 installer"));
    progress = new QProgressBar();
    progress->setFormat("Downloaded %vK of %mK");
    QWidget *downWidget = new QWidget;
    QHBoxLayout *downLayout = new QHBoxLayout(downWidget);
    downLayout->setContentsMargins(0, 0, 0, 0);
    downLayout->addWidget(progress);
    QPushButton *cancel = new QPushButton("Cancel");
    downLayout->addWidget(cancel);
    layout->addWidget(downWidget);
    downloaderWidget->show();

    latestInstallerSize = QFileInfo(installerName).size();

    qDebug() << "downloading" << url;
    networkReply = manager->get(request);
    QObject::connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressChanged(qint64,qint64)));
    QObject::connect(cancel, SIGNAL(clicked(bool)), this, SLOT(cancelUpdate()));
}

void Updater::downloadProgressChanged(qint64 current, qint64 total) {
    progress->setMaximum(total / 1000);
    progress->setValue(current / 1000);

    if (total == latestInstallerSize && networkReply && total) {
        cancelUpdate();
        QMessageBox::information(downloaderWidget, "Update", "Latest version is already downloaded.\nRerunning installer!");
        QDesktopServices::openUrl(QUrl::fromLocalFile(installerName));
    }
}

void Updater::cancelUpdate() {
    QNetworkReply *tmp = networkReply;
    networkReply = NULL;
    tmp->abort();
    downloaderWidget->close();
}

void Updater::runSetup(QNetworkReply *newVersion) {
    if (!newVersion->bytesAvailable()) {
        cancelUpdate();
        return;
    }

    QByteArray installer = newVersion->readAll();
    if (installer.size() < 100000) {
        qDebug() << "Installer is too small. Aborting";
        return;
    }

    QFile exe(installerName);
    if (!exe.open(QIODevice::WriteOnly))
        QMessageBox::information(NULL, "Error", "Can't write to temp: " + installerName);
    else {
        exe.write(installer);
        exe.close();
        qDebug() << "Running setup: " << installerName;
        QDesktopServices::openUrl(QUrl::fromLocalFile(installerName));
    }

    cancelUpdate();
}
