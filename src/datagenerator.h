#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <data.h>

#include <QPair>

class DataGenerator
{
    DataGenerator();

public:
    static Data generateRandomPhrase();
    static Data generate(QString phrase);
    static void fillRay(QVector<double> &ray, QPair<int, int> range);
};

#endif // DATAGENERATOR_H
