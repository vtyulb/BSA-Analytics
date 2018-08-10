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
    explicit KnownNoise();
    ~KnownNoise();

    bool contains(double);

private:
    Ui::KnownNoise *ui;
    QVector<double> doubles;

    void save();
    void reload();

private slots:
    void add();
    void remove();
};

#endif // KNOWNNOISE_H
