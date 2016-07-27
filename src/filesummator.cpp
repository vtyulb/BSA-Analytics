#include "filesummator.h"

#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDir>

#include <reader.h>
#include <startime.h>
#include <data.h>
#include <datadumper.h>
#include <pulsarworker.h>

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

    Reader reader;
    Data multifile;
    Data coefficients;
    bool multifileInited = false;
    QString path = "non-blank";
    while (path != "" || fileNames.count()) {
        printf("Please enter path to data (blank line to stop): ");
        path = input.readLine();
        if (path != "")
            findFiles(path, fileNames, extensions);

        if (fileNames.size() && path != "")
            printf("\nTotal %d files to process\n", fileNames.size());

        if (!multifileInited && fileNames.size() > 0) {
            printf("Reading first file (%s) ... ", fileNames[0].toUtf8().constData());
            fflush(stdout);

            multifile = reader.readBinaryFile(fileNames[0]);

            printf("readed.\n");
            printf("Memory allocation for multifile... ");
            fflush(stdout);

            multifile.npoints *= 25;
            multifile.init();
            multifileInited = true;

            for (int i = 0; i < multifile.modules; i++)
                for (int j = 0; j < multifile.channels; j++)
                    for (int k = 0; k < multifile.rays; k++)
                        for (int u = 0; u < multifile.npoints; u++)
                            multifile.data[i][j][k][u] = 0;

            coefficients = multifile;
            coefficients.fork(); // linux style

            printf("memory allocated.\n");
            printf("Process seems to be all right, beginning to work\n");

        } else if (!multifileInited && !fileNames.size()) {
            printf("Did not found any valid files\n");
            continue;
        }

        if (path != "")
            continue;

        for (int i = 0; i < fileNames.size(); i++) {
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);
            Data data = reader.readBinaryFile(fileNames[i]);
            processData(data);

            double realPart;
            QString time = StarTime::StarTime(data, 0, &realPart);
    //        int h, m, s;
    //        sscanf(time.toUtf8().data(), "%d:%d:%d", &h, &m, &s);
    //        int startPoint = (h * 3600 + m * 60 + s + realPart) / data.oneStep;
            int startPoint = realPart / data.oneStep;

            for (int module = 0; module < data.modules; module++)
                for (int channel = 0; channel < data.channels; channel++)
                    for (int ray = 0; ray < data.rays; ray++)
                        for (int j = 0; j < data.npoints; j++)
                            if (data.data[module][channel][ray][j] != 0)
                        {
                            multifile.data[module][channel][ray][startPoint + j] += data.data[module][channel][ray][j];
                            coefficients.data[module][channel][ray][startPoint + j] += 1;
                        }


            data.releaseData();
        }

        if (fileNames.size()) {
            fileNames.clear();
            path = "test";
        }

        printf("\n\n");
    }

    if (!multifileInited) {
        printf("Zero work done.\n");
        printf("Exiting from this cruel world...\n");
        exit(0);
    }

    for (int module = 0; module < multifile.modules; module++)
        for (int channel = 0; channel < multifile.channels; channel++)
            for (int ray = 0; ray < multifile.rays; ray++)
                for (int i = 0; i < multifile.npoints; i++) {
                    float c = coefficients.data[module][channel][ray][i];
                    if (c > 0.1)
                        multifile.data[module][channel][ray][i] /= c;
                }

    printf("All files were processed\n");

    QFile f;
    QString name;
    while (1) {
        printf("Please enter path to save multifile: ");
        fflush(stdout);
        name = input.readLine();
        f.setFileName(name);
        if (f.open(QIODevice::WriteOnly))
            break;
        else
            printf("Can't open file '%s' for write\n", name.toUtf8().constData());
    }

    printf("Dumping multifile to %s\n", name.toUtf8().constData());
    DataDumper::dump(multifile, f);
}

void FileSummator::processData(Data &data) {
    // Hello pulsarworker::clearAveraNge(), i know you are here
    for (int module = 0; module < data.modules; module++)
        for (int ray = 0; ray < data.rays; ray++)
          {
            const int step =  INTERVAL / data.oneStep;
            for (int channel = 0; channel < data.channels; channel++) {
                for (int i = 0; i < data.npoints; i += step)
                    PulsarWorker::subtract(data.data[module][channel][ray] + i, std::min(step, data.npoints - i));

                double noise = 0;
                for (int i = 0; i < data.npoints; i++)
                    noise += pow(data.data[module][channel][ray][i], 2);

                noise /= data.npoints;
                noise = pow(noise, 0.5);

                const int little = 15;

                for (int i = 0; i < data.npoints - little; i += little) {
                    double sum = 0;
                    for (int j = i; j < i + little; j++)
                        sum += data.data[module][channel][ray][j];

                    sum /= little;
                    sum = fabs(sum);

                    if (sum > noise * 2) {
        //                qDebug() << "clearing stair" << i;

                        for (int j = std::max(i - little * 5, 0); j < i + 60 / data.oneStep && j < data.npoints; j++)
                            data.data[module][channel][ray][j] = 0;

                        break;
                    }
                }

                noise = 0;
                for (int i = 0; i < data.npoints; i++)
                    noise += pow(data.data[module][channel][ray][i], 2);

                noise /= data.npoints;
                noise = pow(noise, 0.5);

                const int maximumNoise = 4;

                float *res = data.data[module][channel][ray];
                for (int i = 0; i < data.npoints; i++)
                    if (res[i] >  noise * maximumNoise)
                        res[i] = noise * maximumNoise;
                    else if (res[i] < -noise * maximumNoise)
                        res[i] = -noise * maximumNoise;

            }
          }
}

void FileSummator::findFiles(QString path, QStringList &names, const QStringList &extensions) {
    QDir dir(path);
    printf("scanning %s\n", path.toUtf8().constData());
    QFileInfoList files = dir.entryInfoList();
    for (int i = 0; i < files.size(); i++)
        if (files[i].isDir() && files[i].fileName() != "." && files[i].fileName() != "..")
            findFiles(files[i].absoluteFilePath(), names, extensions);
        else if (files[i].isReadable() && files[i].fileName() != "multifile.pnt") {
            for (int j = 0; j < extensions.size(); j++)
                if (files[i].fileName().endsWith(extensions[j])) {
                    names.push_back(files[i].absoluteFilePath());
                    break;
                }
        }
}
