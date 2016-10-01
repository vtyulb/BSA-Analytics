#include "filesummator.h"

#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDir>

#include <reader.h>
#include <startime.h>
#include <data.h>
#include <fourier.h>
#include <datadumper.h>
#include <pulsarworker.h>

#include <algorithm>

const int PC_ = 150; // point count
const int CuttingPC = 2048;
const int CuttingPCLong = 16384;

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

    bool cutter = false;
    printf("Do you want to dump cutted files? [y/N] ");
    if (input.readLine().toUpper() == "Y") {
        cutter = true;
        while (true) {
            printf("Please enter path to save cutted files: ");
            cutterPath = input.readLine();
            QDir dir(cutterPath);
            cutterPath = dir.absolutePath() + "/result/";
            if (dir.mkdir("result") || QDir(cutterPath).exists()) {
                loadCuttingState();
                break;
            }
        }

        printf("Path [%s] accepted\n\n", cutterPath.toUtf8().constData());
    }

    PC = cutter ? CuttingPC : PC_;
    if (cutter && extensions.contains("pnthr")) {
        PC = CuttingPCLong;
        longData = true;
    }

    Reader reader;
    Data multifile;
    Data coefficients;
    bool multifileInited = false;
    QString path = "non-blank";
    while (path != "" || fileNames.count() || cutter) {
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
            multifile.releaseData();
            if (!longData)
                multifile.init();

            multifileInited = true;

            if (!longData)
            for (int i = 0; i < multifile.modules; i++)
                for (int j = 0; j < multifile.channels; j++)
                    for (int k = 0; k < multifile.rays; k++)
                        for (int u = 0; u < multifile.npoints; u++)
                            multifile.data[i][j][k][u] = 0;


            noises.resize(multifile.npoints / PC * 2);
            numberOfPieces.resize(multifile.npoints / PC * 2);
            numberOfPieces.fill(0);

            if (!longData) {
                coefficients = multifile;
                coefficients.fork(); // linux style
            }

            printf("memory allocated.\n");
            printf("Process seems to be all right, can work now\n");

        } else if (!multifileInited && !fileNames.size()) {
            printf("Did not found any valid files\n");
            continue;
        }

        if (path != "" && !cutter)
            continue;

        if (fileNames.size())
            printf("\nRunning stage 1 of 2\n");
        stage = 1;
        if (!longData)
        for (int i = 0; i < fileNames.size(); i++) {
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);

            Data data = reader.readBinaryFile(fileNames[i]);
            processData(data, multifile, coefficients);
            data.releaseData();
        }

        if (fileNames.size())
            printf("\nRunning stage 2 of 2\n");
        stage = 2;
        for (int i = 0; i < noises.size(); i++)
            std::sort(noises[i].begin(), noises[i].end());

        for (int i = 0; i < fileNames.size(); i++) {
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);
            if (filesProcessed.contains(fileNames[i]))
                continue;

            Data data = reader.readBinaryFile(fileNames[i]);
            if (longData)
                processLongData(data);
            else
                processData(data, multifile, coefficients);
            data.releaseData();

            filesProcessed.insert(fileNames[i]);
            saveCuttingState();
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

    QVector<float> buf(data.npoints);

    for (int module = 0; module < data.modules; module++)
        for (int ray = 0; ray < data.rays; ray++)
          {
            const int step =  INTERVAL / data.oneStep;
            for (int channel = 0; channel < data.channels; channel++) {
                for (int i = 0; i < data.npoints; i += step)
                    PulsarWorker::subtract(data.data[module][channel][ray] + i, std::min(step, data.npoints - i));

                for (int i = 0; i < data.npoints; i++)
                    buf[i] = data.data[module][channel][ray][i];

                std::sort(buf.begin(), buf.end());

                double noise = 0;
                for (int i = data.npoints * 0.2; i < data.npoints * 0.8; i++)
                    noise += pow(data.data[module][channel][ray][i], 2);

                noise /= data.npoints * 0.8 - data.npoints * 0.2;
                noise = pow(noise, 0.5);

                const double maximumNoise = 1.9;

                float *res = data.data[module][channel][ray];
                for (int i = 0; i < data.npoints; i++)
                    if (res[i] >  noise * maximumNoise)
                        res[i] = noise * maximumNoise;
                    else if (res[i] < -noise * maximumNoise)
                        res[i] = -noise * maximumNoise;

                for (int j = 0; j < data.npoints / PC - 2; j++) {
                    double realPart;
                    QString time = StarTime::StarTime(data, j * PC, &realPart);
                    int startPoint = realPart / data.oneStep;
                    int offset = PC - startPoint % PC;

                    double noise = 0;
                    for (int k = 0; k < PC; k++)
                        noise += pow(data.data[module][channel][ray][j * PC + k + offset], 2);

                    noise /= PC;
                    noise = pow(noise, 0.5);


                    int point = (startPoint) / PC;

                    if (stage == 1)
                        noises[point].push_back(noise);
                    else if (noises[point][noises[point].size() * 0.8] > noise)
                    // stage == 2
                        if (module == data.modules - 1 && ray == data.rays - 1 && channel == data.channels - 1)
                            dumpCuttedPiece(data, j * PC + offset, (startPoint + offset) / PC);

                        for (int k = 0; k < PC; k++) {
                            multifile.data[module][channel][ray][startPoint + k + offset] += data.data[module][channel][ray][j * PC + k + offset];
                            coefficients.data[module][channel][ray][startPoint + k + offset] += 1;
                        }
                }

            }
          }
}

void FileSummator::processLongData(Data &data) {
    // Hello FileSummator::processLongData(), i know you are here

    QVector<float> buf(data.npoints);

    for (int module = 0; module < data.modules; module++)
        for (int ray = 0; ray < data.rays; ray++)
          {
            const int step =  INTERVAL / data.oneStep;
            for (int channel = 0; channel < data.channels; channel++) {
                for (int i = 0; i < data.npoints; i += step)
                    PulsarWorker::subtract(data.data[module][channel][ray] + i, std::min(step, data.npoints - i));

                for (int i = 0; i < data.npoints; i++)
                    buf[i] = data.data[module][channel][ray][i];

                std::sort(buf.begin(), buf.end());

                double noise = 0;
                for (int i = data.npoints * 0.2; i < data.npoints * 0.8; i++)
                    noise += pow(data.data[module][channel][ray][i], 2);

                noise /= data.npoints * 0.8 - data.npoints * 0.2;
                noise = pow(noise, 0.5);

                const double maximumNoise = 1.9;

                float *res = data.data[module][channel][ray];
                for (int i = 0; i < data.npoints; i++)
                    if (res[i] >  noise * maximumNoise)
                        res[i] = noise * maximumNoise;
                    else if (res[i] < -noise * maximumNoise)
                        res[i] = -noise * maximumNoise;

                for (int j = 0; j < data.npoints / PC - 2; j++) {
                    double realPart;
                    QString time = StarTime::StarTime(data, j * PC, &realPart);
                    int startPoint = realPart / data.oneStep;
                    int offset = PC - startPoint % PC;

                    double noise = 0;
                    for (int k = 0; k < PC; k++)
                        noise += pow(data.data[module][channel][ray][j * PC + k + offset], 2);

                    noise /= PC;
                    noise = pow(noise, 0.5);


                    int point = (startPoint) / PC;

                    if (module == data.modules - 1 && ray == data.rays - 1 && channel == data.channels - 1)
                        dumpCuttedPiece(data, j * PC + offset, (startPoint + offset) / PC);
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

void FileSummator::dumpCuttedPiece(const Data &data, int startPoint, int pieceNumber) {
    if (CuttingPC != PC && CuttingPCLong != PC)
        return;

    double realSeconds;
    StarTime::StarTime(data, startPoint, &realSeconds);

    QMap<QString, QString> headerAddition;
    headerAddition["native_datetime"] = data.name;
    headerAddition["star_time"] = QString::number(realSeconds, 'g', 10);

    Data res = data;
    if (!longData) {
        res.npoints = PC;
    //    res.modules = 1;
    //    res.channels = 1;
    //    res.rays = 1;
        res.fork();

        for (int module = 0; module < res.modules; module++)
            for (int channel = 0; channel < res.channels; channel++)
                for (int ray = 0; ray < res.rays; ray++)
                    for (int i = 0; i < CuttingPC; i++)
                        res.data[module][channel][ray][i] = data.data[module][channel][ray][startPoint + i];
    } else {
        res.npoints = PC / 2;
        res.channels = 1;
        res.fork();
        QVector<float> dt(PC / 2, 0);
        QVector<float> tmp = dt;
        for (int module = 0; module < data.modules; module++)
            for (int ray = 0; ray < data.rays; ray++) {
                tmp.fill(0);
                dt.fill(0);
                for (int channel = 0; channel < data.channels - 1; channel++) {
                    Fourier::FFTAnalysis(data.data[module][channel][ray] + startPoint, tmp.data(), PC, PC / 2);
                    for (int i = 0; i < PC / 2; i++)
                        dt[i] += tmp[i];
                }

                for (int i = 0; i < PC / 2; i++)
                    res.data[module][0][ray][i] = dt[i];
            }
    }

    numberOfPieces[pieceNumber]++;
    QDir().mkpath(cutterPath + "/" + QString::number(pieceNumber));
    QFile f(cutterPath + "/" + QString::number(pieceNumber) + "/" + QString::number(numberOfPieces[pieceNumber]) + ".pnt");
    f.open(QIODevice::WriteOnly);
    DataDumper::dump(res, f, headerAddition);
    res.releaseData();
}

void FileSummator::loadCuttingState() {
    QFile f(cutterPath + "state.txt");
    if (f.open(QIODevice::ReadOnly)) {
        printf("\nFound previous cutting data\n");
        printf("Restoring state...\n");
        QTextStream stream(&f);
        stream.readLine();
        numberOfPieces.resize(500);
        for (int i = 0; i < 500; i++)
            stream >> numberOfPieces[i];

        while (!stream.atEnd())
            filesProcessed.insert(stream.readLine().replace("\n", ""));

        printf("State loaded. Total %d files processed already\n\n", filesProcessed.size() - 1);
    }
}

void FileSummator::saveCuttingState() {
    QFile f(cutterPath + "state.txt");
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream stream(&f);
        stream << "Last state change: " << QDateTime::currentDateTime().toString() << "\n";
        for (int i = 0; i < 500; i++)
            stream << numberOfPieces[i] << " ";

        stream << "\n";

        for (QSet<QString>::Iterator i = filesProcessed.begin(); i != filesProcessed.end(); i++)
            stream << (*i) << "\n";

        f.close();
    }
}
