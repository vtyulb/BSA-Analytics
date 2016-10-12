#ifndef PULSARLIST_H
#define PULSARLIST_H

#include <QWidget>
#include <QVector>
#include <QTime>
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

        void closeEvent(QCloseEvent*);
        QString getJName(int module, int ray, QTime time);

    private slots:
        void selectionChanged();

    signals:
        void switchData(Data &data);
};

#endif // PULSARLIST_H
