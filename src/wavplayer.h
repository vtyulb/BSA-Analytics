#ifndef WAVPLAYER_H
#define WAVPLAYER_H

#include <QVector>

class WavPlayer
{
private:
    WavPlayer();

public:
    static void play(QVector<double>, int doubling);
};

#endif // WAVPLAYER_H
