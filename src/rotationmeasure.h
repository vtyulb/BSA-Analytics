#ifndef ROTATIONMEASURE_H
#define ROTATIONMEASURE_H

#include <QDialog>

namespace Ui {
class RotationMeasure;
}

class RotationMeasure : public QDialog
{
    Q_OBJECT

public:
    explicit RotationMeasure(QWidget *parent = 0);
    ~RotationMeasure();

private:
    Ui::RotationMeasure *ui;

private slots:
    void setStairFile();
    void setFile();
    void run();
};

#endif // ROTATIONMEASURE_H
