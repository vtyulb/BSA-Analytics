#include "calculationpool.h"

QThreadPool *CalculationPool::pool() {
    static QThreadPool *res = new QThreadPool;
    return res;
}
