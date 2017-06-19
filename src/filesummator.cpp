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

namespace {
    void clearLine() {
        printf("\r");
        for (int i = 0; i < 120; i++) printf(" ");
    }
}

FileSummator::FileSummator() {}

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

    bool stairsSearch = false;
    bool cutter = false;
    bool transientSearch = false;
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
        stairsNameOverride = "./noises.pnt";
    } else {
        printf("Do you want to start stairs search? [y/N] ");
        if (input.readLine().toUpper() == "Y")
            stairsSearch = true;
        else {
            printf("Do you want to start transient search [y/N] ");
            if (input.readLine().toUpper() == "Y") {
                transientSearch = true;
                while (true) {
                    printf("Please enter path to save database: ");
                    cutterPath = input.readLine();
                    QDir dir(cutterPath);
                    cutterPath = dir.absolutePath() + "/result/";
                    if (dir.mkdir("result"))
                        if (QFile(cutterPath + "transients").open(QIODevice::WriteOnly))
                            break;
                }

                printf("Path [%s] accepted\n\n", cutterPath.toLocal8Bit().constData());
            } else {
                printf("So, I can't help you then\n");
                return;
            }
        }
    }

    PC = cutter ? CuttingPC : PC_;
    if ((cutter || transientSearch) && extensions.contains("pnthr")) {
        PC = CuttingPCLong;
        longData = true;
    }

    Reader reader;
    Data multifile;
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
            stairs = multifile;
            stairs.npoints = 0;

            initStairs(stairs, stairsNames);

            printf("... readed.\n");
            printf("Memory allocation for multifile... ");
            fflush(stdout);

            multifile.npoints *= 25;
            multifile.releaseData();
            multifileInited = true;

            numberOfPieces.resize(multifile.npoints / PC * 2);
            numberOfPieces.fill(0);

            printf("memory allocated.\n");
            printf("Process seems to be all right, can work now\n");

        } else if (!multifileInited && !fileNames.size()) {
            printf("Did not found any valid files\n");
            continue;
        }

        if (path != "" && !cutter)
            continue;

        for (int i = 0; i < fileNames.size(); i++) {
            clearLine();
            printf("\rReading file %d of %d [%s]", i + 1, fileNames.size(), fileNames[i].toUtf8().constData());
            fflush(stdout);
            if (filesProcessed.contains(fileNames[i]))
                continue;

            Data data;
            data.previousLifeName = QFileInfo(fileNames[i]).fileName();
            if (stairsSearch) {
                if (data.hourFromPreviousLifeName() == 1 ||
                    data.hourFromPreviousLifeName() == 5 ||
                    data.hourFromPreviousLifeName() == 9 ||
                    data.hourFromPreviousLifeName() == 13 ||
                    data.hourFromPreviousLifeName() == 17 ||
                    data.hourFromPreviousLifeName() == 21 || !longData)
                {
                    if (stairsNames.contains(data.previousLifeName))
                        continue;

                    data = reader.readBinaryFile(fileNames[i]);
                    int start = 0;
                    int end = 0;

                    if (data.modules == 6 && data.rays == 8 && findStair(data, start, end)) {
                        Settings::settings()->detectStair(data, start, end);
                        stairsNames.push_back(QFileInfo(fileNames[i]).fileName());
                        addStair(stairs);
                        dumpStairs(stairs, stairsNames);
                    }

                    data.releaseData();
                }

                continue;
            }

            data = reader.readBinaryFile(fileNames[i]);
            if (!data.isValid())
                continue;

            Reader().repairWrongChannels(data);

            if (transientSearch) {
                transientProcess(data);
                data.releaseData();
                continue;
            }

            if (longData)
                processLongData(data);
            else
                processData(data);

            if (i % 100 == 0 /*&& stairsSearch*/)
                dumpStairs(stairs, stairsNames);

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

    if (stairsSearch) {
        checkStairs(stairs, stairsNames);
        sortStairs(stairs, stairsNames);
        dumpStairs(stairs, stairsNames);
        printf("Stairs sorted and dumped to %s\n", getStairsName(stairs).toUtf8().constData());
        exit(0);
    } else if (cutter) {
        sortStairs(stairs, stairsNames);
        dumpStairs(stairs, stairsNames);
        printf("Noises dumped to %s", getStairsName(stairs).toUtf8().constData());
    }

    if (!multifileInited) {
        printf("Zero work done.\n");
        printf("Exiting from this cruel world...\n");
        exit(0);
    }

    printf("\nAll files were processed\n");
}

bool FileSummator::processData(Data &data) {
    // Hello pulsarworker::clearAveraNge(), i know you are here

    if (!Settings::settings()->loadStair())
        return false;

    for (int module = 0; module < data.modules; module++)
        for (int channel = 0; channel < data.channels; channel++)
            for (int ray = 0; ray < data.rays; ray++)
                for (int i = 0; i < data.npoints; i++)
                    data.data[module][channel][ray][i] /= Settings::settings()->getStairHeight(module, ray, channel) / 2100.0;

    QVector<float> buf(data.npoints);
    noises.clear();
    noises.resize(data.modules);
    for (int module = 0; module < data.modules; module++)
        noises[module].resize(data.channels);

    for (int module = 0; module < data.modules; module++)
        for (int ray = 0; ray < data.rays; ray++) {
            const int step =  INTERVAL / data.oneStep;
            for (int channel = 0; channel < data.channels; channel++) {
                for (int i = 0; i < data.npoints; i += step)
                    PulsarWorker::subtract(data.data[module][channel][ray] + i, std::min(step, data.npoints - i));
                //-------------------------------------------------
                for (int i = 0; i < data.npoints; i++)
                    buf[i] = data.data[module][channel][ray][i];

                std::sort(buf.begin(), buf.end());

                double mn = buf[data.npoints * 0.1];
                double mx = buf[data.npoints * 0.9];

                float *res = data.data[module][channel][ray];
                for (int i = 0; i < data.npoints; i++)
                    if (res[i] > mx)
                        res[i] = mx;
                    else if (res[i] < mn)
                        res[i] = mn;

                //-------------------------------------------------

                for (int j = 0; j < data.npoints / PC - 2; j++) {
                    double realPart;
                    StarTime::StarTime(data, j * PC, &realPart);
                    int startPoint = realPart / data.oneStep;
                    int offset = PC - startPoint % PC;
                    if (module == data.modules - 1 && ray == data.rays - 1 && channel == data.channels - 1)
                        dumpCuttedPiece(data, j * PC + offset, (startPoint + offset) / PC);
                }
            }
        }

    return true;
}

void FileSummator::processLongData(Data &data) {
    // Hello FileSummator::processLongData(), i know you are here
    qDebug()  << "It won't work!!! contact <vtyulb@vtyulb.ru>";

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
        res.fork();

        for (int module = 0; module < res.modules; module++)
            for (int channel = 0; channel < res.channels; channel++) {
                noises[module][channel].clear();
                for (int ray = 0; ray < res.rays; ray++) {
                    double noise = 0;
                    for (int i = 0; i < CuttingPC; i++) {
                        res.data[module][channel][ray][i] = data.data[module][channel][ray][startPoint + i];
                        noise += res.data[module][channel][ray][i] * (double)res.data[module][channel][ray][i];
                    }

                    noise /= CuttingPC;
                    noise = sqrt(noise);
                    noises[module][channel].push_back(noise);
                }
            }

        QString newNoise = data.name + "/" + QString::number(pieceNumber);
        if (!stairsNames.contains(newNoise)) {
            stairsNames.push_back(newNoise);
            addStair(stairs);
        }
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
    QDir().mkpath(cutterPath + "/" + QString::asprintf("%03d", pieceNumber));
    QFile f(cutterPath + "/" + QString::asprintf("%03d", pieceNumber) + "/" + QString::asprintf("%04d", numberOfPieces[pieceNumber]) + ".pnt");
    f.open(QIODevice::WriteOnly);
    DataDumper::dump(res, f, headerAddition);
    res.releaseData();
}

bool FileSummator::dumpTransient(const QVector<double> &data, const Data &rawData, int startPoint, int pieceNumber, int module, int ray, int dispersion, double snr) {
    double realSeconds;
    StarTime::StarTime(rawData, startPoint, &realSeconds);

    QMap<QString, QString> headerAddition;
    headerAddition["native_datetime"] = rawData.name;
    headerAddition["star_time"] = QString::number(realSeconds, 'g', 10);
    headerAddition["module"] = QString::number(module + 1);
    headerAddition["ray"] = QString::number(ray + 1);
    headerAddition["rays"] = "1";
    headerAddition["point"] = QString::number(startPoint);
    headerAddition["dispersion"] = QString::number(dispersion);
    headerAddition["star_time"] = StarTime::StarTime(rawData, startPoint);
    headerAddition["snr"] = QString::number(snr);

    double v1 = rawData.fbands[0];
    double v2 = rawData.fbands[1];

    int start = startPoint + dispersion * 4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * 32 / rawData.oneStep - 10;
    int end = startPoint + 17;

    Data res = rawData;
    res.modules = 1;
    res.channels = 33;
    res.rays = 1;
    res.npoints = end - start;
    res.init();

    QVector<double> chk(end - start, 0);
    double chkMx = 0;
    for (int i = start; i < end; i++)
        for (int j = 0; j < rawData.channels - 1; j++) {
            chk[i - start] += rawData.data[module][j][ray][i];
            chkMx = std::max(chkMx, chk[i - start] / (rawData.channels - 1));
        }

    if (data[startPoint] < chkMx * TRANSIENT_FILTER_AMPLIFICATION_TRESH) {
        res.releaseData();
        return false;
    }

    for (int i = start; i < end; i++)
        res.data[0][32][0][i - start] = data[i];

    for (int i = start; i < end; i++)
        for (int channel = 0; channel < 32; channel++)
            res.data[0][channel][0][i - start] = rawData.data[module][channel][ray][i];

    numberOfPieces[pieceNumber]++;
    QDir().mkpath(cutterPath + "/" + QString::asprintf("%03d", pieceNumber));
    QFile f(cutterPath + "/" + QString::asprintf("%03d", pieceNumber) + "/" + QString::asprintf("%04d", numberOfPieces[pieceNumber]) + ".pnt");
    f.open(QIODevice::WriteOnly);
    DataDumper::dump(res, f, headerAddition);
    res.releaseData();

    return true;
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

void FileSummator::addStair(Data &stairs) {
    static int realSize = 0;
    Data res = stairs;
    res.npoints++;
    if (realSize < res.npoints) {
        res.npoints *= 2;
        res.init();

        realSize = res.npoints;
        res.npoints = stairs.npoints + 1;
    }

    int v = res.npoints - 1;
    for (int module = 0; module < res.modules; module++)
        for (int channel = 0; channel < res.channels; channel++)
            for (int ray = 0; ray < res.rays; ray++) {
                if (stairs.npoints && res.data[module][channel][ray] != stairs.data[module][channel][ray])
                    memcpy(res.data[module][channel][ray], stairs.data[module][channel][ray], stairs.npoints * sizeof(float));

                res.data[module][channel][ray][v] = Settings::settings()->getStairHeight(module, ray, channel);
                if (stairsNameOverride != "")
                    res.data[module][channel][ray][v] = noises[module][channel][ray];
            }

    if (stairs.npoints && res.data[0][0][0] != stairs.data[0][0][0])
        stairs.releaseData();
    stairs = res;
}

QString FileSummator::getStairsName(const Data &stairs) {
    if (stairsNameOverride != "")
        return stairsNameOverride;

    if (stairs.isLong())
        return "./LongStairs.pnthr";
    else
        return "./ShortStairs.pnt";
}

void FileSummator::dumpStairs(const Data &stairs, const QStringList &stairsNames) {
    QString stairsResName = getStairsName(stairs);

    QString names = stairsNames[0];
    for (int i = 1; i < stairsNames.size(); i++)
        names += "," + stairsNames[i];

    QFile out(stairsResName);
    out.open(QIODevice::WriteOnly);

    QMap<QString, QString> m;
    m["stairs_names"] = names;

    DataDumper::dump(stairs, out, m);
}

void FileSummator::initStairs(Data &stairs, QStringList &names) {
    QString stairsResName = getStairsName(stairs);
    Data tmp = Reader().readBinaryFile(stairsResName);
    if (tmp.isValid()) {
        stairs = tmp;
        names = Settings::settings()->getLastHeader()["stairs_names"].split(",");
        while (names.size() > stairs.npoints)
            names.removeLast();

        Data tmp1 = Reader().readBinaryFile(stairsResName + ".1");
        if (tmp1.isValid()) {
            qDebug() << names.size() << " stairs in first file";
            qDebug() << "found second stairs file";
            names += Settings::settings()->getLastHeader()["stairs_names"].split(",");
            stairs.npoints += tmp1.npoints;
            qDebug() << tmp1.npoints << " stairs in second file";
            stairs.init();
            for (int module = 0; module < stairs.modules; module++)
                for (int ray = 0; ray < stairs.rays; ray++)
                    for (int channel = 0; channel < stairs.channels; channel++) {
                        for (int i = 0; i < tmp.npoints; i++)
                            stairs.data[module][channel][ray][i] = tmp.data[module][channel][ray][i];

                        for (int i = 0; i < tmp1.npoints; i++)
                            stairs.data[module][channel][ray][i+tmp.npoints] = tmp1.data[module][channel][ray][i];
                    }
        }

        qDebug() << "found previous stairs file at" << stairsResName;
        qDebug() << names.size() << "stairs extracted";
    } else
        qDebug() << "previous stairs search not found. Starting from scratch";
}

void FileSummator::sortStairs(Data &stairs, QStringList &names) {
    qDebug() << "sorting stairs";
    QVector<QPair<QDateTime, int>> hlp;
    for (int i = 0; i < names.size(); i++)
        hlp.push_back(QPair<QDateTime, int>(stringDateToDate(names[i]), i));

    std::sort(hlp.begin(), hlp.end());

    Data newStairs = stairs;
    newStairs.fork();
    QStringList newNames = names;

    for (int i = 0; i < names.size(); i++) {
        newNames[i] = names[hlp[i].second];
        for (int module = 0; module < stairs.modules; module++)
            for (int ray = 0; ray < stairs.rays; ray++)
                for (int channel = 0; channel < stairs.channels; channel++)
                    newStairs.data[module][channel][ray][i] = stairs.data[module][channel][ray][hlp[i].second];
    }

    stairs.releaseData();
    stairs = newStairs;
    names = newNames;
}

bool FileSummator::findStair(Data &data, int &start, int &end) {
    QVector<QPair<double, int> > values;
    for (int i = 0; i < data.npoints; i++)
        values.push_back(QPair<double, int>(data.data[0][data.channels - 1][0][i], i));

    std::sort(values.begin(), values.end());
    start = values[0].second;
    end = values[0].second;

    int count = data.isLong() ? 780 : 90;
    values.resize(count);
    for (int i = 0; i < count; i++) {
        start = std::min(start, values[i].second);
        end = std::max(end, values[i].second);
    }

    if (end - start < count * 2 && data.data[0][0][0][(start+end)/2] - values[0].first > 0.2)
        return true;
    else
        return false;
}

void FileSummator::checkStairs(Data &stairs, QStringList &names) {
    qDebug() << "searching for bad data";
    for (int i = 0; i < stairs.npoints; i++) {
        bool good = true;
        QString reason;
        if (!names[i].contains("_N1_") && !names[i].contains("_N2_")) {
            good = false;
            reason = "Does not have ground flag (_N1_ or _N2_)";
        }

        for (int module = 0; module < stairs.modules; module++)
            for (int channel = 0; channel < stairs.channels; channel++)
                for (int ray = 0; ray < stairs.rays; ray++)
                    if (good) {
                        if (stairs.data[module][channel][ray][i] < 0.05) {
                            good = false;
                            reason = QString("Data at module %1 ray %2 channel %3 is too small").arg(module+1).arg(ray+1).arg(channel+1);
                        } else if (stairs.data[module][channel][ray][i] > 100) {
                            good = false;
                            reason = QString("Data at module %1 ray %2 channel %3 is too big").arg(module+1).arg(ray+1).arg(channel+1);
                        }
                    }

        if (!good) {
            qDebug() << "file" << names[i] << "excluded from stairs for reason:" << reason;
            for (int module = 0; module < stairs.modules; module++)
                for (int channel = 0; channel < stairs.channels; channel++)
                    for (int ray = 0; ray < stairs.rays; ray++)
                        stairs.data[module][channel][ray][i] = stairs.data[module][channel][ray][stairs.npoints - 1];

            names[i] = names[stairs.npoints - 1];
            names.removeLast();
            stairs.npoints--;
            i--;
        }
    }
}

QVector<double> FileSummator::applyDispersion(Data &data, int D, int module, int ray) {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];
    double mxd = (4.1488) * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D;
    mxd *= -data.channels;
    mxd /= data.oneStep;

    QVector<double> res(data.npoints, 0);

    for (int i = 0; i < data.npoints - mxd; i++)
        for (int j = 0; j < data.channels - 1; j++) {
            int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * D * j / data.oneStep + 0.5);
            res[i] += data.data[module][j][ray][std::max(i + dt, 0)];
        }

    for (int i = data.npoints - mxd; i < data.npoints; i++)
        for (int j = 0; j < data.channels - 1; j++)
            res[i] += data.data[module][j][ray][i];

    for (int i = 0; i < res.size(); i++)
        res[i] /= (data.channels - 1);

    return res;
}

bool FileSummator::transientCheckAmplification(const Data &data, int point, int module, int ray, int dispersion) {
    double v1 = data.fbands[0];
    double v2 = data.fbands[1];

    double sum = 0;
    float mx = 0;
    for (int i = 0; i < data.channels - 1; i++) {
        int dt = int(4.1488 * (1e+3) * (1 / v2 / v2 - 1 / v1 / v1) * dispersion * i / data.oneStep + 0.5);
        sum += data.data[module][i][ray][point + dt];
        mx = std::max(data.data[module][i][ray][point + dt], mx);
    }

    return mx * TRANSIENT_AMPLIFICATION_TRESH < sum;
}

void FileSummator::transientProcess(Data &data) {
    if (!Settings::settings()->loadStair())
        qDebug() << "can't normalize current data";

    for (int module = 0; module < data.modules; module++)
        for (int channel = 0; channel < data.channels; channel++)
            for (int ray = 0; ray < data.rays; ray++)
                for (int i = 0; i < data.npoints; i++)
                    data.data[module][channel][ray][i] /= Settings::settings()->getStairHeight(module, ray, channel) / 2100.0;

    QVector<int> transientsCount(500, 0);
    int total = 0;
    for (int module = 0; module < data.modules; module++)
        for (int ray = 0; ray < data.rays; ray++) {
            printf(".");
            fflush(stdout);

            const int step =  INTERVAL / data.oneStep;
            for (int channel = 0; channel < data.channels; channel++) {
                for (int i = 0; i < data.npoints; i += step)
                    PulsarWorker::subtract(data.data[module][channel][ray] + i, std::min(step, data.npoints - i));
            }

            for (int disp = 2; disp <= 100; disp++) {
                QVector<double> res = applyDispersion(data, disp, module, ray);
                double noise = 0;
                for (int i = 0; i < res.size(); i++)
                    noise += res[i] * res[i];
                noise /= res.size();
                noise = pow(noise, 0.5);

                for (int i = 1000; i < res.size() - 1000; i++)
                    if (res[i] / noise > TRANSIENT_THRESH)
                        if (res[i] / data.data[module][32][ray][i] > TRANSIENT_AMPLIFICATION_TRESH) {
                            double realPart;
                            StarTime::StarTime(data, i, &realPart);
                            int startPoint = realPart / data.oneStep;
                            int offset = PC - startPoint % PC;
                            int block = (startPoint + offset) / PC;
                            if (transientsCount[block] == -1)
                                continue;

                            bool dumped = dumpTransient(res, data, i, block, module, ray, disp, res[i] / noise);
                            if (dumped) {
                                transientsCount[block]++;
                                total++;
                            }


                            if (transientsCount[block] > TRANSIENT_COUNT_TRESH) {
                                printf("X");
                                int last = numberOfPieces[block];
                                numberOfPieces[block] -= transientsCount[block];
                                total -= transientsCount[block];
                                transientsCount[block] = -1;
                                for (int j = numberOfPieces[block] + 1; j <= last; j++) {
                                    QString trash = cutterPath + "/" + QString::asprintf("%03d", block) + "/" + QString::asprintf("%04d", j) + ".pnt";
                                    QFile(trash).remove();
                                }

                                break;
                            }

                            i += 200;
                        }
            }
        }

    printf("%d objects found\n", total);
}
