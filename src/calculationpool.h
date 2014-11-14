#ifndef CALCULATIONPOOL_H
#define CALCULATIONPOOL_H

#include <QThreadPool>

class CalculationPool {
    public:
        static QThreadPool *pool();
};

#endif // CALCULATIONPOOL_H
