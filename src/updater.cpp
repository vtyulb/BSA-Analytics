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
const QUrl downloadUrl("https://bsa.vtyulb.ru/BSA-Analytics-x64.exe");

Updater::Updater(QObject *parent) : QObject(parent)
{
    latestInstallerSize = QFileInfo(installerName).size();
    manager = new QNetworkAccessManager;
}

void Updater::download() {
    QNetworkRequest request(downloadUrl);

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(dumpSetup(QNetworkReply*)));

    downloaderWidget = new QWidget;
    downloaderWidget->setWindowTitle("Updating BSA-Analytics");
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

    qDebug() << "downloading" << downloadUrl;
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
        runSetup();
    }
}

void Updater::cancelUpdate() {
    if (!networkReply)
        return;

    QNetworkReply *tmp = networkReply;
    networkReply = NULL;
    tmp->abort();
    downloaderWidget->close();
}

void Updater::dumpSetup(QNetworkReply *newVersion) {
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
        exe.setPermissions(QFileDevice::ExeOwner | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        exe.write(installer);
        exe.close();
        runSetup();
    }

    cancelUpdate();
}

void Updater::runSetup() {
    qDebug() << "Running setup: " << installerName;
    QDesktopServices::openUrl(QUrl::fromLocalFile(installerName));
}

void Updater::checkForUpdates() {
    QNetworkRequest request(downloadUrl);
    manager->head(request);
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkFinished(QNetworkReply*)));
}

void Updater::checkFinished(QNetworkReply *reply) {
    QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkFinished(QNetworkReply*)));
    int size = reply->header(QNetworkRequest::ContentLengthHeader).toInt();
    if (size != latestInstallerSize) {
        if (QMessageBox::question(NULL, "Updater",
                                  "New update available!\n"
                                  "Should I download it now?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            download();
    }
}
