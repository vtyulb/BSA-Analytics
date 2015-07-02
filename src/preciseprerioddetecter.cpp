#include <preciseprerioddetecter.h>
#include <pulsarworker.h>
#include <reader.h>
#include <settings.h>

void PrecisePreriodDetecter::detect(QString file1, QString file2, QString file3, int module, int ray, int dispersion, double period, QTime time) {

    Settings::settings()->setPreciseSearch(true);
    Settings::settings()->setPeriod(period);
    Settings::settings()->setIntellectualFilter(false);
    Settings::settings()->setTime(time);


    Reader r;
    PulsarWorker worker1(module, ray, dispersion, r.readBinaryFile(file1));
    worker1.run();

    PulsarWorker worker2(module, ray, dispersion, r.readBinaryFile(file2));
    worker2.run();

    PulsarWorker worker3(module, ray, dispersion, r.readBinaryFile(file3));
    worker3.run();


}
