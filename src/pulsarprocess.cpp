#include "pulsarprocess.h"

PulsarProcess::PulsarProcess(QString file, QObject *parent):
    QThread(parent),
    file(file)
{
    Reader reader;
    data = reader.readBinaryFile(file);
}

void PulsarProcess::run() {
    for (int i = 0; i < 33; i++)
        for (int j = 0; j < 60; j++)
            noise[i][j] = 0;


}
