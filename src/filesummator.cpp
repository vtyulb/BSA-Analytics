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

#include <algorithm>

const int PC = 150; // point count

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


            noises.resize(multifile.npoints / PC * 2);

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

        goodSigma = -1;
        sigmas.clear();

        if (fileNames.size())
            printf("Running stage 1 of 2\n");
        stage = 1;
        for (int i = 0; i < fileNames.size(); i++) {
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);
            Data data = reader.readBinaryFile(fileNames[i]);
            processData(data, multifile, coefficients);

            data.releaseData();
        }

        std::sort(sigmas.begin(), sigmas.end());

        goodSigma = sigmas[sigmas.size() * 0.1];

        if (fileNames.size())
            printf("\nRunning stage 2 of 2\n");
        stage = 2;
        for (int i = 0; i < noises.size(); i++)
            std::sort(noises[i].begin(), noises[i].end());

        for (int i = 0; i < fileNames.size(); i++) {
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);
            Data data = reader.readBinaryFile(fileNames[i]);
            processData(data, multifile, coefficients);
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

void FileSummator::processData(Data &data, Data &multifile, Data &coefficients) {
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

                /*if (goodSigma < 0) {
                    sigmas.push_back(noise);
                    continue;
                } else if (noise < goodSigma)
                    continue;*/


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


                for (int j = 0; j < data.npoints / PC - 1; j++) {
                    double noise = 0;
                    for (int k = 0; k < PC; k++)
                        noise += pow(data.data[module][channel][ray][j * PC + k], 2);

                    noise /= PC;
                    noise = pow(noise, 0.5);


                    double realPart;
                    QString time = StarTime::StarTime(data, j * PC, &realPart);
                    int startPoint = realPart / data.oneStep;

                    int point = startPoint / PC;
                    int moduleStartEnd = 24 * 60 * 60 / data.oneStep;

                    if (stage == 1)
                        noises[point].push_back(noise);
                    else if (noises[point][noises[point].size() / 2] > noise)
                    // stage == 2
                        for (int k = 0; k < PC; k++) {
                            multifile   .data[module][channel][ray][(startPoint + k) % moduleStartEnd] += data.data[module][channel][ray][j * PC + k];
                            coefficients.data[module][channel][ray][(startPoint + k) % moduleStartEnd] += 1;
                        }
                }

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
