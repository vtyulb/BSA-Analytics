#include "pulsarlist.h"

#include <pulsarreader.h>

#include <QDebug>
#include <QKeyEvent>
#include <QTimer>
#include <QSettings>
#include <QTableWidget>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>

PulsarList::PulsarList(QWidget *parent) :
    QTableWidget(NULL)
{
    QObject::setParent(parent);

    QObject::connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    QObject::connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));

    QAction *showUTCtime = new QAction("Show UTC time", this);
    QObject::connect(showUTCtime, SIGNAL(triggered(bool)), this, SLOT(showTime()));
    addAction(showUTCtime);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setColumnCount(6);

    setAttribute(Qt::WA_DeleteOnClose);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QStringList header;
    header << "time" << "module" << "ray";
    if (Settings::settings()->fourierAnalytics())
        header << "sigma";
    else
        header << "dispersion";
    header << "period" << "snr";
    setHorizontalHeaderLabels(header);

    setStyleSheet("QMenu::item:selected{border:1px solid red;}");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setWindowTitle("Pulsar list");

    show();
}

void PulsarList::init(Pulsars pl, bool removeBadData) {
    pulsars = pl;

    pulsarsIndex.clear();
    setRowCount(pulsars->size());
    for (int i = 0; i < pulsars->size(); i++) {
        if (i % 100 == 0)
            emit progress(90 * i / pulsars->size());

        setItem(i, 0, new QTableWidgetItem(getPulsarJName(pulsars->at(i).module, pulsars->at(i).ray, pulsars->at(i).nativeTime)));
        setItem(i, 1, new QTableWidgetItem(QString::number(pulsars->at(i).module)));
        setItem(i, 2, new QTableWidgetItem(QString::number(pulsars->at(i).ray)));
        setItem(i, 3, new QTableWidgetItem(QString::number(pulsars->at(i).dispersion)));
        setItem(i, 4, new QTableWidgetItem(QString::number(pulsars->at(i).period, 'f', 5)));
        setItem(i, 5, new QTableWidgetItem(QString::number(pulsars->at(i).snr, 'f', 1)));

        if (pulsars->at(i).filtered)
            for (int j = 0; j < columnCount(); j++)
                item(i, j)->setBackgroundColor(QColor("lightgray"));

        if (!pulsars->at(i).filtered && !pulsars->at(i).showInTable && Settings::settings()->fourierAnalytics())
            for (int j = 0; j < columnCount(); j++)
                item(i, j)->setBackgroundColor(QColor(200, 100, 100));

        if (pulsars->at(i).fourierDuplicate)
            item(i, 0)->setBackgroundColor(QColor(255, 255, 150));

        pulsarsIndex.push_back(i);
    }

    if (removeBadData) {
        int v = 0;
        for (int i = 0; i < pulsars->size(); i++) {
            if (i % 100 == 0)
                emit progress(90 + 5 * i / pulsars->size());

            if (pulsars->at(i).showInTable) {
                for (int j = 0; j < columnCount(); j++)
                    setItem(v, j, new QTableWidgetItem(*(item(i, j))));

                pulsarsIndex[v] = i;
                v++;
            }

        }

        setRowCount(v);
    }

    resizeColumnsToContents();

    hide();
    show();

    if (pulsars->size()) {
        selectRow(0);
        selectionChanged();
    }
}

void PulsarList::closeEvent(QCloseEvent *) {
    saveSettings();
}

PulsarList::~PulsarList() {
    saveSettings();
    qDebug() << "Pulsar list destroyed";
}

void PulsarList::saveSettings() {
    qDebug() << "Pulsar list saving settings";
    QSettings().setValue("pulsar-list-geometry", saveGeometry());
    qApp->quit();
}

void PulsarList::selectionChanged() {
    if (selectionModel()->selection().indexes().size())
        if (selectionModel()->selection().indexes().at(0).row() < pulsars->size()) {
            currentPulsar = &(*pulsars)[pulsarsIndex[selectionModel()->selection().indexes().at(0).row()]];
            emit switchData(currentPulsar->data);
        }
}

void PulsarList::showTime() {
    QMessageBox::information(this, "Peak UTC time", currentPulsar->UTCtime());
}

QSize PulsarList::sizeHint() const {
    QSize res = QTableWidget::sizeHint();
    res.setWidth(horizontalHeader()->length() + 5 + verticalHeader()->width() + verticalScrollBar()->width());
    return res;
}

void PulsarList::keyPressEvent(QKeyEvent *event) {
    if (!selectionModel()->selection().indexes().size())
        return;

    int current = selectionModel()->selection().indexes().at(0).row();
    if ((!pulsars->at(pulsarsIndex[current]).filtered || pulsars->at(pulsarsIndex[current]).fourierDuplicate) &&
           (rowCount() > 10))
    {
        if (event->key() == Qt::Key_PageDown) {
            current++;
            if (current >= rowCount())
                return;

            while (sameFile(current, current - 1) && current != rowCount() - 1)
                current++;

            selectRow(current);
            event->accept();
        } else if (event->key() == Qt::Key_PageUp) {
            current--;
            if (current < 0)
                return;

            while (sameFile(current, current + 1) && current)
                current--;

            selectRow(current);
            event->accept();
        } else
            QTableWidget::keyPressEvent(event);
    } else
        QTableWidget::keyPressEvent(event);
}

bool PulsarList::sameFile(int f1, int f2) {
    const Pulsar &p1 = pulsars->at(pulsarsIndex[f1]);
    const Pulsar &p2 = pulsars->at(pulsarsIndex[f2]);
    return p1.module == p2.module &&
            p1.ray == p2.ray &&
            p1.data.previousLifeName == p2.data.previousLifeName;
}

QSize PulsarList::minimumSizeHint() const {
    return sizeHint();
}
