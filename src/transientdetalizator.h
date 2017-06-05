#ifndef TRANSIENTDETALIZATOR_H
#define TRANSIENTDETALIZATOR_H


#include <QString>
#include <QTime>

class TransientDetalizator
{
public:
    static void run(int module, int ray, QTime time, QString file);
};

#endif // TRANSIENTDETALIZATOR_H
