#include "filesummator.h"

#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDir>

#include <reader.h>
#include <data.h>

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
        data.releaseData();
    }
    printf("\nAll files were processed\n");
    printf("Dumping multifile to NOT COMPLETED\n");
    fflush(stdout);

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
