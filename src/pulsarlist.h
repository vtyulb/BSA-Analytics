#ifndef PULSARLIST_H
#define PULSARLIST_H

#include <QTableWidget>
#include <QVector>
#include <QTime>
#include <QPoint>
#include <QSize>

#include <pulsar.h>

class PulsarList : public QTableWidget
{
    Q_OBJECT

    public:
        explicit PulsarList(QWidget *parent);
        ~PulsarList();

        void init(Pulsars pl, bool removeBadData);

    private:
        Pulsars pulsars;
        Pulsar *currentPulsar;

        QVector<int> pulsarsIndex;

        void closeEvent(QCloseEvent *);
        QSize sizeHint() const;
        QSize minimumSizeHint() const;

        void saveSettings();

    private slots:
        void selectionChanged();
        void showTime();

    signals:
        void switchData(Data &data);
};

#endif // PULSARLIST_H
