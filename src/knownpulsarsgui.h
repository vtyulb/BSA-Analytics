#ifndef KNOWNPULSARSGUI_H
#define KNOWNPULSARSGUI_H

#include <QWidget>
#include <QVector>

#include "knownpulsar.h"

namespace Ui {
class KnownPulsarsGUI;
}

class KnownPulsarsGUI : public QWidget
{
    Q_OBJECT

public:
    explicit KnownPulsarsGUI(QWidget *parent = 0);
    ~KnownPulsarsGUI();

private:
    Ui::KnownPulsarsGUI *ui;
    QVector<KnownPulsar> pulsars;

    void reload();
    void dump();

private slots:
    void add();
    void remove();
};

#endif // KNOWNPULSARSGUI_H
