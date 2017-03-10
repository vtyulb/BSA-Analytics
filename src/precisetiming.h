#ifndef PRECISETIMING_H
#define PRECISETIMING_H

#include <QDialog>

namespace Ui {
    class PreciseTiming;
}

class PreciseTiming : public QDialog
{
    Q_OBJECT

public:
    explicit PreciseTiming(QWidget *parent = 0);
    ~PreciseTiming();

private:
    Ui::PreciseTiming *ui;

private slots:
    void setFile1();
    void setFile2();
    void setFile3();

    void run();
};

#endif // PRECISETIMING_H
