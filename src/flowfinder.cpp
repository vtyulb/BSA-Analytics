#include "flowfinder.h"
#include <settings.h>

void FlowFinder::find(QString fileName) {
    Settings::settings()->setFlowFinder(true);
}
