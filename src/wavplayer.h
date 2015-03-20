#ifndef WAVPLAYER_H
#define WAVPLAYER_H

#include <QVector>

class WavPlayer
{
private:
    WavPlayer();

public:
    static void play(QVector<double>);
};

#endif // WAVPLAYER_H
