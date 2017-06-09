#ifndef TRANSIENTPERIOD_H
#define TRANSIENTPERIOD_H

#include <data.h>

#include <QDialog>

namespace Ui {
    class TransientPeriod;
}

class TransientPeriod : public QDialog
{
    Q_OBJECT

public:
    explicit TransientPeriod(QWidget *parent = 0);
    ~TransientPeriod();

    void setText(QString);

private:
    Ui::TransientPeriod *ui;

private slots:
    void generateData();

signals:
    void dataGenerated(Data&);
};

#endif // TRANSIENTPERIOD_H
