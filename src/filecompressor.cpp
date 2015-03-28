#include "filecompressor.h"

#include <QFileInfoList>
#include <QDir>
#include <pulsarreader.h>

void FileCompressor::compress(QString name) {
    QFileInfoList list = QDir(name).entryInfoList(QDir::Dirs);
    for (int i = 0; i < list.size(); i++)
        if (list[i].fileName() != "." && list[i].fileName() != "..")
            compress(list[i].absoluteFilePath());

    list = QDir(name).entryInfoList(QDir::Files);
    Pulsars pulsars;
    for (int i = 0; i < list.size(); i++)
        (*pulsars) += *PulsarReader::ReadPulsarFile(list[i].absoluteFilePath());

    if (pulsars->size())
        FileCompressor::dump(pulsars, QDir(name).dirName() + ".tar.pulsar");
}

void FileCompressor::dump(Pulsars pulsars, QString name) {

}
