#ifndef PULSARLIST_H
#define PULSARLIST_H

#include <QWidget>
#include <QVector>
#include <pulsar.h>

namespace Ui {
    class PulsarList;
}

class PulsarList : public QWidget
{
    Q_OBJECT

    public:
        explicit PulsarList(QString fileName, QWidget *parent = 0);
        ~PulsarList();

    private:
        Ui::PulsarList *ui;
        Pulsars pulsars;

    private slots:
        void selectionChanged(int, int, int, int);

    signals:
        void switchData(Data &data);
};

#endif // PULSARLIST_H
