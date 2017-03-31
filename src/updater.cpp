#include "updater.h"

#include <QApplication>
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
#include <QSettings>
#include <QSysInfo>
#include <QThread>
#include <QLabel>
#include <QUuid>

const QString installerName = QDir::tempPath() + "/BSA-Analytics-x64.exe";
const QString downloadUrl("https://bsa.vtyulb.ru/BSA-Analytics-x64.exe");

Updater::Updater(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager;
}

void Updater::download() {
    QNetworkRequest request(getInstallerUrl());

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

    qDebug() << "downloading" << getInstallerUrl();
    networkReply = manager->get(request);
    QObject::connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressChanged(qint64,qint64)));
    QObject::connect(cancel, SIGNAL(clicked(bool)), this, SLOT(cancelUpdate()));
}

void Updater::downloadProgressChanged(qint64 current, qint64 total) {
    progress->setMaximum(total / 1000);
    progress->setValue(current / 1000);
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

void Updater::checkForUpdates(bool silence) {
    silentMode = silence;
    bool stable = QSettings().value("Stable", QVariant(true)).toBool();
    if (QSettings().value("LastTimeCheckedForUpdates").toDate().daysTo(QDate::currentDate()) < 1 + stable * 3 && silentMode) {
        qDebug() << "already checked for updates today";
        return;
    }

    if (stable)
        QSettings().setValue("LastTimeCheckedForUpdates", QDate::currentDate());

    QString uuid = QSettings().value("UUID").toString();
    if (uuid == "") {
        uuid = QUuid::createUuid().toString().replace("{", "").replace("}", "").replace("-", "").left(6);
        QSettings().setValue("UUID", uuid);
    }

    QNetworkRequest request(getInstallerUrl());
    request.setRawHeader("User-Agent", "BSA-Analytics {" + uuid.toUtf8() + "} from " + getSystemInfo());
    manager->head(request);
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkFinished(QNetworkReply*)));
}

void Updater::checkFinished(QNetworkReply *reply) {
    QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkFinished(QNetworkReply*)));
    QDateTime latestInstaller = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
    qDebug() << "Latest installer was created at " << latestInstaller.toString();
    long long secs = QFileInfo(qApp->applicationFilePath()).lastModified().secsTo(latestInstaller);
    if (secs > 3600) {
        qDebug() << "Your version is outdated by" << secs << "seconds";
        if (QMessageBox::question(NULL, "Updater",
                                  "New update available!\n" +
                                  QString("Your version is outdated by %1 days %2 hours.\n").arg(secs/3600/24).arg(secs/3600%24) +
                                  "Do you want to update now?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            download();
        else
            QSettings().setValue("LastTimeCheckedForUpdates", QDate::currentDate());
    } else {
        qDebug() << "Installed version is latest";
        if (!silentMode)
            QMessageBox::information(NULL, "Updater", "Installed version is latest!");
    }
}

QByteArray Updater::getSystemInfo() {
    QString info = QSysInfo::prettyProductName() +
                   ", " + QString::number(QThread::idealThreadCount()) + " cores" +
                   ", " + getRAMcount();

    qDebug() << "System info:" << info;
    return info.toUtf8();
}

QByteArray Updater::getRAMcount() {
    long long ram = getNativeRAMcount();
    ram /= 1024; // in KB now
    ram /= 1024; // in MB now
    return (QString::number(ram / 1000.0, 'f', 1) + "G").toUtf8();
}

QUrl Updater::getInstallerUrl() {
    if (QSettings().value("Stable", QVariant()).toBool())
        return QUrl(QString(downloadUrl).replace("-x64", "-stable-x64"));
    else
        return QUrl(downloadUrl);
}

#ifdef Q_OS_LINUX
#include <unistd.h>
long long Updater::getNativeRAMcount() {
    long long pages = sysconf(_SC_PHYS_PAGES);
    long long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#else
#include <windows.h>
long long Updater::getNativeRAMcount() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif

