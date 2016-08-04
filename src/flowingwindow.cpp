#include "flowingwindow.h"

#include <reader.h>
#include <datadumper.h>

void FlowingWindow::run(QString input, QString output, QString number) {
    Reader r;
    Data data = r.readBinaryFile(input);
    Data res = data;
    res.fork();

    const int PC = number.toInt();
    for (int module = 0; module < data.modules; module++)
        for (int channel = 0; channel < data.channels; channel++)
            for (int ray = 0; ray < data.rays; ray++)
                for (int i = 0; i < data.npoints - PC; i++) {
                    res.data[module][channel][ray][i] = 0;
                    for (int k = 0; k < PC; k++)
                        res.data[module][channel][ray][i] +=
                            data.data[module][channel][ray][i + k];
                }


    QFile f(output);
    f.open(QIODevice::WriteOnly);
    DataDumper::dump(res, f);
}
