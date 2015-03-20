#include "wavplayer.h"
#include <stdio.h>
#include <QDebug>
#include <QProcess>
#include <QStringList>
#include <pulsarworker.h>

namespace {
    struct Header {
        const char chunkId[4] = {'R', 'I', 'F', 'F'};
        int chunkSize;
        const char format[4] = {'W', 'A', 'V', 'E'};
        const char subchunkId[4] = {'f', 'm', 't', ' '};
        const int subChunk1Size = 16;
        const short audioFormat = 1;
        const short numChannels = 1;
        int sampleRate;
        int byteRate;
        short blockAlign;
        short bitsPerSample;
        const char subchunk2Id[4] = {'d', 'a', 't', 'a'};
        int subchunk2Size;
    };

    void subtract(double *res, int size) {
        double a = 0;
        double b = 0;
        for (int i = 0; i < size / 2; i++)
            a += res[i] / (size / 2);

        for (int i = 1; i <= size / 2; i++)
            b += res[size - i] / (size / 2);

        for (int i = 0; i < size; i++)
            res[i] -= (b - a) * i / size + a;
    }
}

void WavPlayer::play(QVector<double> data) {
    subtract(data.data(), data.size());

    const int doubling = 100;

    Header header;
    header.chunkSize = 44 - 8 + data.size() * 2 * doubling;
    header.sampleRate = doubling * 10;
    header.bitsPerSample = 16;
    header.byteRate = header.sampleRate * header.bitsPerSample / 8;
    header.blockAlign = 16;
    header.subchunk2Size = header.chunkSize - (44 - 8);

    FILE *fout = fopen("wave.wav", "w");
    fwrite(&header, sizeof(header), 1, fout);

    double min = 100;
    double max = 0;
    for (int i = 0; i < data.size(); i++)
        if (data[i] < min)
            min = data[i];
        else if (data[i] > max)
            max = data[i];

    for (int i = 0; i < data.size(); i++) {
        for (int j = 0; j < doubling; j++) {
            short val = (data[i] - min)/(max-min) * 32000 * (j % 2 * 2 - 1);
            fwrite(&val, sizeof(val), 1, fout);
        }
    }

    fclose(fout);

//    QStringList l;
//    l << "wave.wav";
//    QProcess::startDetached("aplay", l);
    qDebug() << "wav file created";
}
