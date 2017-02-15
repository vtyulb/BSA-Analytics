#ifndef PULSARLIST_H
#define PULSARLIST_H

#include <QWidget>
#include <QVector>
#include <QTime>
#include <QPoint>

#include <pulsar.h>

namespace Ui {
    class PulsarList;
}

class PulsarList : public QWidget
{
    Q_OBJECT

    public:
        explicit PulsarList(QString fileName, Pulsars pl = 0, bool removeBadData = false, QWidget *parent = 0);
        ~PulsarList();

    private:
        Ui::PulsarList *ui;
        Pulsars pulsars;
        Pulsar *currentPulsar;

        void closeEvent(QCloseEvent*);

        QVector<int> pulsarsIndex;

    private slots:
        void selectionChanged();
        void showTime();

    signals:
        void switchData(Data &data);
};

#endif // PULSARLIST_H
