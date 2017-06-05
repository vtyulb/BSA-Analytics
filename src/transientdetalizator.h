#ifndef TRANSIENTDETALIZATOR_H
#define TRANSIENTDETALIZATOR_H

#include <data.h>

#include <QString>
#include <QTime>

class TransientDetalizator
{
public:
    static void run(int module, int ray, QTime time, QString file, Data data);
};

#endif // TRANSIENTDETALIZATOR_H
