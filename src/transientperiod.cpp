#include "transientperiod.h"
#include "ui_transientperiod.h"

#include <math.h>

#include <QString>
#include <QTextEdit>

TransientPeriod::TransientPeriod(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransientPeriod)
{
    ui->setupUi(this);

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(generateData()));
}

void TransientPeriod::generateData() {
    QVector<double> diffs;
    QStringList lst = ui->textEdit->toPlainText().split("\n");
    for (int i = 0; i < lst.size(); i++) {
        QVector<int> cur;
        QStringList curStrs = lst[i].split(" ");
        for (int j = 0; j < curStrs.size(); j++) {
            bool ok = false;
            int point = curStrs[j].toInt(&ok);
            if (ok)
                cur.push_back(point);
        }

        for (int j = 0; j < cur.size(); j++)
            for (int k = j + 1; k < cur.size(); k++)
                diffs.push_back(abs(cur[j] - cur[k]) * 0.0124928);
    }

    printf("Found %d diffs:", diffs.size());
    for (int i = 0; i < diffs.size(); i++)
        printf(" %lf", diffs[i]);

    printf("\n");
    fflush(stdout);

    int total = 20000;
    Data res;
    res.modules = 1;
    res.rays = 1;
    res.channels = 1;
    res.npoints = total;
    res.init();

    res.data[0][0][0][0] = 0;

    double glMx = 0;
    for (int i = 1; i < total; i++) {
        double period = i / 1000.0;
        double mx = 0;
        for (int j = 0; j < diffs.size(); j++)
            mx = std::max(mx, fabs(diffs[j] - int(diffs[j] / period + 0.5) * period));

        glMx = std::max(glMx, mx);
        res.data[0][0][0][i] = mx;
    }

    for (int i = 0; i < total; i++)
        res.data[0][0][0][i] = glMx - res.data[0][0][0][i];

    emit dataGenerated(res);
}

TransientPeriod::~TransientPeriod()
{
    delete ui;
}
