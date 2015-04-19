#ifndef KNOWNNOISE_H
#define KNOWNNOISE_H

#include <QWidget>
#include <QVector>

namespace Ui {
class KnownNoise;
}

class KnownNoise : public QWidget
{
    Q_OBJECT

public:
    explicit KnownNoise(QWidget *parent = 0);
    ~KnownNoise();

    bool contains(double);

private:
    Ui::KnownNoise *ui;
    QVector<double> doubles;

    bool goodDoubles(double, double);
    void save();
    void reload();

private slots:
    void add();
    void remove();
};

#endif // KNOWNNOISE_H
