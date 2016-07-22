#include "filesummator.h"

#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDir>

#include <reader.h>
#include <startime.h>
#include <data.h>
#include <datadumper.h>

FileSummator::FileSummator()
{    
}

void FileSummator::run() {

    QTextStream input(stdin);
    QStringList extensions;
    QStringList fileNames;

    printf("Please enter eligible extensions comma-separated [pnt,pnthr] [pnt by default]: ");
    QString ext = input.readLine();
    if (ext != "")
        extensions = ext.split(",");
    else
        extensions << "pnt";


    printf("Eligible names:\n");
    for (int i = 0; i < extensions.size(); i++)
        printf("*.%s\n", extensions[i].toUtf8().constData());


    QString path = "non-blank";
    while (path != "") {
        printf("Please enter path to data (blank line to stop): ");
        path = input.readLine();
        if (path != "")
            findFiles(path, fileNames, extensions);
    }

    printf("\nTotal %d files to process\n", fileNames.size());
    printf("Reading first file (%s) ... ", fileNames[0].toUtf8().constData());
    fflush(stdout);

    Reader reader;
    Data multifile = reader.readBinaryFile(fileNames[0]);

    printf("readed.\n");
    printf("Memory allocation for multifile... ");
    fflush(stdout);

    multifile.npoints *= 25;
    multifile.init();

    for (int i = 0; i < multifile.modules; i++)
        for (int j = 0; j < multifile.channels; j++)
            for (int k = 0; k < multifile.rays; k++)
                for (int u = 0; u < multifile.npoints; u++)
                    multifile.data[i][j][k][u] = 0;

    Data coefficients = multifile;
    coefficients.fork(); // linux style

    printf("memory allocated.\n");
    printf("Process seems to be all right, beginning to work\n");
    for (int i = 0; i < fileNames.size(); i++) {
        printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
        fflush(stdout);
        Data data = reader.readBinaryFile(fileNames[i]);

        double realPart;
        QString time = StarTime::StarTime(data, 0, &realPart);
        int h, m, s;
        sscanf(time.toUtf8().data(), "%d:%d:%d", &h, &m, &s);
        int startPoint = (h * 3600 + m * 60 + s + realPart) / data.oneStep;

        for (int module = 0; module < data.modules; module++)
            for (int channel = 0; channel < data.channels; channel++)
                for (int ray = 0; ray < data.rays; ray++)
                    for (int j = 0; j < data.npoints; j++) {
                        multifile.data[module][channel][ray][startPoint + j] += data.data[module][channel][ray][j];
                        coefficients.data[module][channel][ray][startPoint + j] += 1;
                    }


        data.releaseData();
    }

    for (int module = 0; module < multifile.modules; module++)
        for (int channel = 0; channel < multifile.channels; channel++)
            for (int ray = 0; ray < multifile.rays; ray++)
                for (int i = 0; i < multifile.npoints; i++) {
                    float c = coefficients.data[module][channel][ray][i];
                    if (c > 0.1)
                        multifile.data[module][channel][ray][i] /= c;
                }

    printf("\nAll files were processed\n");

    QString name = "multifile.pnt";
    printf("Dumping multifile to %s\n", name.toUtf8().constData());
    fflush(stdout);

    DataDumper::dump(multifile, name);
}

void FileSummator::findFiles(QString path, QStringList &names, const QStringList &extensions) {
    QDir dir(path);
    printf("scanning %s\n", path.toUtf8().constData());
    QFileInfoList files = dir.entryInfoList();
    for (int i = 0; i < files.size(); i++)
        if (files[i].isDir() && files[i].fileName() != "." && files[i].fileName() != "..")
            findFiles(files[i].absoluteFilePath(), names, extensions);
        else if (files[i].isReadable()) {
            for (int j = 0; j < extensions.size(); j++)
                if (files[i].fileName().endsWith(extensions[j])) {
                    names.push_back(files[i].absoluteFilePath());
                    break;
                }
        }
}
