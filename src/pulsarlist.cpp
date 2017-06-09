#include "pulsarlist.h"

#include <pulsarreader.h>
#include <spectredrawer.h>
#include <mainwindow.h>

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

const QColor knownPulsarColor(0, 255, 0);
const QColor fourierDuplicateColor(255, 255, 150);
const QColor notShowInTableColor(200, 100, 100);
const QColor filteredColor("lightgray");
const QColor markedColor(50, 50, 240);

PulsarList::PulsarList(QWidget *parent) :
    QTableWidget(NULL)
{
    QObject::setParent(parent);

    QObject::connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    QObject::connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));

    QAction *showUTCtime = new QAction("Show UTC time", this);
    QObject::connect(showUTCtime, SIGNAL(triggered()), this, SLOT(showTime()));
    addAction(showUTCtime);

    QAction *showComment = new QAction("Show comment", this);
    QObject::connect(showComment, SIGNAL(triggered()), this, SLOT(showComment()));
    addAction(showComment);

    QAction *markObject = new QAction("Mark/Unmark object", this);
    QObject::connect(markObject, SIGNAL(triggered()), this, SLOT(markObject()));
    addAction(markObject);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setColumnCount(6);

    setAttribute(Qt::WA_DeleteOnClose);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QStringList header;
    header << "time" << "module" << "ray";
    if (Settings::settings()->fourierAnalytics() && !Settings::settings()->transientAnalytics())
        header << "sigma";
    else
        header << "dispersion";
    header << "period" << "snr";
    setHorizontalHeaderLabels(header);

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
                item(i, j)->setBackgroundColor(filteredColor);

        if (!pulsars->at(i).showInTable && Settings::settings()->fourierAnalytics() && !Settings::settings()->transientAnalytics())
            for (int j = 1; j < columnCount(); j++)
                item(i, j)->setBackgroundColor(notShowInTableColor);

        if (pulsars->at(i).fourierDuplicate)
            item(i, 0)->setBackgroundColor(fourierDuplicateColor);

        if (pulsars->at(i).isKnownPulsar)
            for (int j = 1; j < 6; j++)
                item(i, j)->setBackgroundColor(knownPulsarColor);

        pulsarsIndex.push_back(i);
    }

    if (Settings::settings()->transientAnalytics())
        hideColumn(4);

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
    if (selectionModel()->selection().indexes().size()) {
        int selectedRow = selectionModel()->selection().indexes().at(0).row();
        if (selectedRow < pulsarsIndex.size() && selectedRow >= 0 && pulsarsIndex[selectedRow] < pulsars->size()) {
            currentPulsar = &(*pulsars)[pulsarsIndex[selectedRow]];

            static bool normalColors = true;
            if (currentPulsar->isKnownPulsar && normalColors) {
                setStyleSheet("QTableView::item:selected{background-color: rgb(0,200,0); color: rgb(0,0,0)};");
                normalColors = false;
            } else if (!currentPulsar->isKnownPulsar && !normalColors) {
                setStyleSheet("");
                normalColors = true;
            }

            static bool markedColors = false;
            if (currentPulsar->marked && !markedColors) {
                setStyleSheet("QTableView::item:selected{background-color: rgb(0,0,200); color: rgb(0,0,0)};");
                markedColors = true;
            } else if (!currentPulsar->marked && markedColors) {
                setStyleSheet("");
                markedColors = false;
            }

            if (Settings::settings()->transientAnalytics()) {
                if (currentPulsar->filtered && currentPulsar->data.channels == 33) {
                    Settings::settings()->getSpectreDrawer()->show();
                    drawSpectre(*currentPulsar);
                } else {
                    SpectreDrawer *global = Settings::settings()->getSpectreDrawer();
                    if (global)
                        global->hide();
                }
            }

            emit switchData(currentPulsar->data);
            Settings::settings()->getMainWindow()->update();
        }
    }
}

void PulsarList::showTime() {
    QMessageBox::information(this, "Peak UTC time", currentPulsar->UTCtime());
}

void PulsarList::showComment() {
    if (currentPulsar->isKnownPulsar)
        QMessageBox::information(this, "Known pulsar info", "This is well-known pulsar\n" + currentPulsar->knownPulsarComment);
    else if (currentPulsar->knownPulsarComment != "")
        QMessageBox::information(this, "Known noise info", "This is well-known noise\n" + currentPulsar->knownPulsarComment);
    else
        QMessageBox::information(this, "Object info", "This is unknown object!");
}

void PulsarList::markObject() {
    if (!currentPulsar->filtered)
        return;

    currentPulsar->marked = !currentPulsar->marked;
    int row = selectionModel()->selection().indexes().first().row();
    for (int i = 0; i < 6; i++)
        item(row, i)->setBackgroundColor(currentPulsar->marked ? markedColor :
                                         currentPulsar->isKnownPulsar && i > 0 ? knownPulsarColor : filteredColor);

    selectionChanged();
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
    if ((!pulsars->at(pulsarsIndex[current]).filtered || pulsars->at(pulsarsIndex[current]).fourierDuplicate || Settings::settings()->transientAnalytics()) &&
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
            (p1.data.previousLifeName == p2.data.previousLifeName || Settings::settings()->transientAnalytics());
}

void PulsarList::drawSpectre(const Pulsar &pl) {
    Settings::settings()->setDispersion(pl.dispersion);
    Settings::settings()->getSpectreDrawer()->drawSpectre(0, 0, pl.data, QTime(), 9999, 0);
}

QSize PulsarList::minimumSizeHint() const {
    return sizeHint();
}

QString PulsarList::exportObjectsForPeriodDetalization() {
    QMap<QString, QVector<int> > objects;
    for (int i = 0; i < pulsars->size(); i++)
        if (pulsars->at(i).marked)
            objects[pulsars->at(i).data.previousLifeName.split(" ").last()].push_back(pulsars->at(i).firstPoint);

    QString res;
    for (auto i = objects.begin(); i != objects.end(); i++) {
        QVector<int> cur = i.value();
        res += i.key() + ": ";
        for (int j = 0; j < cur.size(); j++)
            res += QString::number(cur[j]) + " ";

        res += "\n";
    }

    return res;
}
