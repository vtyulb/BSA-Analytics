#include "flowingwindow.h"

#include <reader.h>
#include <datadumper.h>

void FlowingWindow::run(QString input, QString output) {
    Reader r;
    Data data = r.readBinaryFile(input);
    Data res = data;
    res.fork();

    for (int module = 0; module < data.modules; module++)
        for (int channel = 0; channel < data.channels; channel++)
            for (int ray = 0; ray < data.rays; ray++)
                for (int i = 2; i < data.npoints - 2; i++)
                    res.data[module][channel][ray][i] =
                            data.data[module][channel][ray][i - 1] +
                            data.data[module][channel][ray][i];
//                            data.data[module][channel][ray][i + 1] +
//                            data.data[module][channel][ray][i - 2];
//                            data.data[module][channel][ray][i + 2];

    QFile f(output);
    f.open(QIODevice::WriteOnly);
    DataDumper::dump(res, f);
}
