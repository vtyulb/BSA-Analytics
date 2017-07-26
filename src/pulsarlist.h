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

    Pulsar *currentPulsar;
    void init(Pulsars pl, bool removeBadData);
    QString exportObjectsForPeriodDetalization();
    void drawSpectre(const Pulsar &pl);

private:
    Pulsars pulsars;

    QVector<int> pulsarsIndex;

    void keyPressEvent(QKeyEvent*);
    void closeEvent(QCloseEvent *);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    bool sameFile(int, int);

    void saveSettings();

    void resetColors(const Pulsar &pulsar, int row);

    void nonblockingSleep(int ms);

public slots:
    void sumUpMarked();

private slots:
    void selectionChanged();
    void showTime();
    void showComment();
    void markObject();
    void deselectAll();
    void findImpulseWidth();

signals:
    void switchData(Data &data);
    void progress(int);
};

#endif // PULSARLIST_H
