#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <qdialogbuttonbox.h>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMessageBox>
#include <QtSvg>
#include <QTimer>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QRgb>

#include <cmath>

#include <startime.h>
#include <settings.h>
#include <wavplayer.h>
#include "nativedrawer.h"

using std::min;

const QString TEMPORARY_SVG_FILE = QDir::tempPath() + "/bsa-analytics.svg";

NativeDrawer::NativeDrawer(const Data &data, QWidget *parent) :
    QWidget(parent),
    allowDrawing(false),
    autoDrawing(true),
    drawAxesFlag(true),
    drawNet(false),
    drawFast(false),
    channel(0),
    module(0),
    art(NULL),
    mousePressed(false),
    verticalLine(-1)
{
    for (int i = 0; i < data.rays; i++)
        rayVisibles.push_back(true);

    QAction *saveImage = new QAction("Save as...", this);
    addAction(saveImage);
    QObject::connect(saveImage, SIGNAL(triggered()), this, SLOT(saveFile()));

    QAction *exportDataToCSVaction = new QAction("Export to CSV...", this);
    addAction(exportDataToCSVaction);
    QObject::connect(exportDataToCSVaction, SIGNAL(triggered()), this, SLOT(exportDataToCSV()));

    QAction *applyMedianFiltration = new QAction("Apply median filtration", this);
    addAction(applyMedianFiltration);
    QObject::connect(applyMedianFiltration, SIGNAL(triggered()), this, SLOT(applyMedianFilter()));

    setMouseTracking(true);
    setMinimumSize(100, 100);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setData(data);
    channel = data.channels - 1;
    resetVisibleRectangle(false);
}

NativeDrawer::~NativeDrawer() {
    delete art;
    data.releaseData();
    qDebug() << "data released";
}

void NativeDrawer::setData(const Data &newData) {
    data = newData;
    Settings::settings()->setLastData(data);
    if (data.modules == 1 && data.rays == 1 && data.npoints < 25000) {
        double sigma = data.sigma;
        data.fork();
        data.sigma = sigma;
    }
}

void NativeDrawer::setRayVisibles(QVector<bool> v) {
    rayVisibles = v;
    nativePaint();
}

void NativeDrawer::paintEvent(QPaintEvent *event) {
    if (width() <= 100 && height() <= 100)
        return;

    QPainter p(this);

    if (art)
        p.drawImage(QRect(0, 0, width(), height()), *art);
    else
        return;

    p.setPen(QColor("black"));
    p.setBrush(QBrush(QColor(0, 50, 200, 100)));
    if (mousePressed) {
        if (Settings::settings()->sourceMode()) {
            mouseRect.setBottom(-2);
            mouseRect.setTop(height()+1);
        }
        p.drawRect(min(mouseRect.left(), mouseRect.right()),
                   min(mouseRect.top(), mouseRect.bottom()),
                   abs(mouseRect.width()),
                   abs(mouseRect.height()));
    } else if (Settings::settings()->sourceMode()) {
        p.drawLine(verticalLine, 0, verticalLine, height());
    }

    p.end();
    event->accept();
}

void NativeDrawer::nativePaint(bool forPrinter, bool svgFormat) {
    if (!allowDrawing)
        return;

    if (!drawing.tryLock())
        return;

    if (width() <= 100 && height() <= 100) {
        drawing.unlock();
        return;
    }

    if (!forPrinter && !svgFormat) {
        delete art;
        art = new QImage(this->width(), this->height(), QImage::Format_RGB32);
    }

    QSvgGenerator *svgGenerator;
    if (svgFormat) {
        delete art;
        art = new QImage(200, 200, QImage::Format_ARGB32);

        svgGenerator = new QSvgGenerator();
        svgGenerator->setFileName(TEMPORARY_SVG_FILE);
        svgGenerator->setSize(QSize(art->width(), art->height()));
        svgGenerator->setViewBox(QRect(0, 0, width(), height()));
        svgGenerator->setTitle(tr("SVG Generator Example Drawing"));
        svgGenerator->setDescription(tr("An SVG drawing created by the SVG Generator "
                                        "Example provided with Qt."));
    }

    QPainter p;
    if (svgFormat)
        p.begin(svgGenerator);
    else
        p.begin(art);

    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("white"));
    p.drawRect(0, 0, art->width(), art->height());
    p.setPen(QPen("black"));

    drawAxes(p);

    int rays = data.rays;
    for (int k = 0; k < data.npoints / 50000 + 1; k++) {
        if (k)
            if (live)
                repaint();

        if (data.npoints > 50000)
            emit progress(5000000*k/(data.npoints + 1));

        for (int j = 0; j < rays; j++) {
            QByteArray c = QByteArray::fromHex(colors[j].toUtf8());
            p.setPen(QColor((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));

            if (rayVisibles[j])
                for (int i = k * 50000 + 1 + drawFast * 5; i < min(data.npoints, (k + 1)*50000 + 1); i += 1 + drawFast * 5) {
                    int x = newCoord(i, 0).x();
                    if (x < 0 || x > art->width())
                        continue;

                    p.drawLine(mirr(newCoord(i, data.data[module][channel][j][i])), mirr(newCoord(i - 1 - drawFast * 5, data.data[module][channel][j][i - 1])));
                }
        }
    }

    if (Settings::settings()->fourierAnalytics())
        fourierDraw(p);

    p.drawText(20, 20, data.message);

    emit progress(100);
    drawAxes(p);
    p.end();

    drawing.unlock();
    if (width() > 100 || height() > 100)
        repaint();
}

void NativeDrawer::fourierDraw(QPainter &p) {
    int original = data.sigma - 0.5;
    if (original > 5)
        for (int harmonic = 1; harmonic <= 5; harmonic++) {
            int point = original * harmonic;
            if (point > data.npoints - 2)
                break;
            QPoint peak = mirr(newCoord(point, data.data[0][0][0][point]));
            QPoint up = peak;
            peak.setY(peak.y() - 15);
            up.setY(0);
            QPoint left = peak;
            left.setY(left.y() - 15);
            QPoint right = left;
            left.setX(left.x() - 7);
            right.setX(right.x() + 7);

            p.setPen(QPen(QBrush("blue"), 2));
            if (harmonic != 1) {
                p.setPen(QPen(QBrush("lightblue"), 1));
            }
            p.drawLine(up, peak);
            p.drawLine(left, peak);
            p.drawLine(right, peak);
        }
}

void NativeDrawer::resetVisibleRectangle(bool repaint, bool resetLeftRight) {
    screens.clear();

    float min = 1e+30;
    float max = -min;

    for (int i = 0; i < data.npoints; i++)
         for (int j = 0; j < data.rays; j++)
//             for (int channel = 0; channel < data.channels; channel++)
                 if (!std::isinf(data.data[module][channel][j][i])) {
                     if (data.data[module][channel][j][i] > max)
                         max = data.data[module][channel][j][i];

                     if (data.data[module][channel][j][i] < min)
                         min = data.data[module][channel][j][i];
                 } else {
                     qDebug() << "error at point" << i << "at ray" << j;
                 }

    float deltaX = data.npoints * 0.08;
    float deltaY = (max - min) * 0.05;

    if (resetLeftRight) {
        screen.setBottomLeft(QPointF(0 - deltaX, min - deltaY));
        screen.setTopRight(QPointF(data.npoints + deltaX, max + deltaY));
    } else {
        screen.setTop(max + deltaY);
        screen.setBottom(min - deltaY);
    }

    if (repaint)
        nativePaint();
}

QPoint NativeDrawer::newCoord(float x, float y) {
    QPoint res;
    res.setX((x - screen.left())*art->width()/(screen.right()-screen.left()));
    res.setY((y - screen.bottom())*art->height()/(screen.top()-screen.bottom()));
    return res;
}

QPointF NativeDrawer::backwardCoord(QPointF p) {
    QPointF res;
    res.setX(screen.left() + (screen.right()-screen.left())*p.x()/art->width());
    res.setY(screen.bottom() + (screen.top()-screen.bottom())*p.y()/art->height());
    return res;
}

void NativeDrawer::mousePressEvent(QMouseEvent *event) {
    if (!drawing.tryLock())
        return;

    mousePressed = true;
    mouseClicked = event->pos();
    mouseRect = QRect(mouseClicked, mouseClicked);
    drawing.unlock();
}

void NativeDrawer::mouseMoveEvent(QMouseEvent *event) {
    if (!art)
        return;

    verticalLine = event->pos().x();
    mouseRect = QRect(mouseClicked, event->pos());
    if (mousePressed || Settings::settings()->sourceMode())
        repaint();

    emit coordsChanged(backwardCoord(mirr(event->pos())));
}

void NativeDrawer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mousePressed)
        return;

    mousePressed = false;

    if (Settings::settings()->sourceMode() && mouseRect.width() > 0) {
        sourceDetect(backwardCoord(mouseRect.topLeft()).x(), backwardCoord(mouseRect.bottomRight()).x());
        return;
    }

    if (abs(mouseRect.width()) < 50 || abs(mouseRect.height()) < 50) {
        repaint();
        return;
    }

    if (Settings::settings()->soundMode()) { //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        qDebug() << "playing";
        int x1 = backwardCoord(mouseRect.topLeft()).x();
        int x2 = backwardCoord(mouseRect.bottomRight()).x();
        QVector<double> sound;
        int ray = 0;
        for (; ray < data.rays; ray++)
            if (rayVisibles[ray])
                break;

        for (int i = x1; i < x2; i++)
            sound.push_back(data.data[module][channel][ray][i]);

        WavPlayer::play(sound, data.oneStep * 1000 + 1);
        return;
    }

    if (mouseClicked.x() > event->pos().x() || mouseClicked.y() > event->pos().y()) {
        if (screens.size())
            screen = screens.pop();
        else {
            resetVisibleRectangle();
            repaint();
            return;
        }
    } else {
        QRectF c;
        //WTF How it works?!
        int top = mouseRect.top();
        int bot = mouseRect.bottom();

        mouseRect.setTop(height() - bot);
        mouseRect.setBottom(height() - top);

        c.setTopLeft(backwardCoord(mouseRect.bottomLeft()));
        c.setBottomRight(backwardCoord(mouseRect.topRight()));

        float deltaY = (c.top() - c.bottom()) * 0.05;
        float deltaX = (c.right() - c.left()) * 0.05;

        c.setTop(c.top() + deltaY);
        c.setBottom(c.bottom() - deltaY);
        c.setLeft(c.left() - deltaX);
        c.setRight(c.right() + deltaX);

        screens.push_back(screen);
        screen = c;
    }

    nativePaint();
    repaint();
}

void NativeDrawer::resizeEvent(QResizeEvent *event) {
    if (autoDrawing)
        nativePaint();

    event->accept();
}

void NativeDrawer::saveFile(QString file) {
    if (file == "") {
        file = QFileDialog::getSaveFileName(this);
        if (file == "")
            return;
    }
    QByteArray ext = file.right(file.size() - file.lastIndexOf('.') - 1).toUtf8();
    if (ext.size() > 6)
        file += ".png";

    if (ext == "svg") {
        bool axisesFlag = drawAxesFlag;
        drawAxesFlag = false;
        nativePaint(false, true);
        QFile(file).remove();
        QFile(TEMPORARY_SVG_FILE).copy(file);
        QFile(TEMPORARY_SVG_FILE).remove();

        drawAxesFlag = axisesFlag;
        nativePaint();
    } else
        art->save(file);
}

void NativeDrawer::setColors(QVector<QString> c) {
    colors = c;
}

void NativeDrawer::drawAxes(QPainter &p) {
    if (!drawAxesFlag || !art)
        return;

    p.setBrush(QBrush(QColor("white")));
    p.setPen(QColor("black"));

    p.drawLine(QPoint(0, art->height() - 1), QPoint(art->width(), art->height() - 1));
    p.drawLine(QPoint(0, 0), QPoint(art->width(), 0));
    for (int i = 50; i < art->width() - 20; i+=50) {
        if (drawNet) {
            QPen backup = p.pen();
            p.setPen(Qt::DotLine);
            p.drawLine(QPoint(i, 0), QPoint(i, height()));
            p.setPen(backup);
        }

        p.drawLine(QPoint(i, art->height()), QPoint(i, art->height() - 6 - (5 + art->height() * drawNet) * (i%250 == 0)));
        p.drawLine(QPoint(i, 0), QPoint(i, 6 + 5 * (i%250 == 0)));
        if (i%250==0 && i < art->width() - 80) {
            drawText(&p, QPoint(i + 1, art->height() - 12), QString::number(backwardCoord(QPoint(i, 0)).x()));
            drawText(&p, QPoint(i + 1, art->height() - 24), StarTime::StarTime(data, backwardCoord(QPointF(i, 0)).x()));
        }
    }

    p.drawLine(QPoint(0, 0), QPoint(0, art->height()));
    p.drawLine(QPoint(art->width() - 1, 0), QPoint(art->width() - 1, art->height()));

    int startPoint = mirr(newCoord(0,0)).y();
    int bigDash = 0;
    if (startPoint < 20 || startPoint > art->height() - 20 || !Settings::settings()->nullOnOYaxis())
        startPoint = 50;
    else {
        bigDash = (-(startPoint / 50) % 5 + 5) % 5;
        startPoint %= 50;
    }

    for (int i = startPoint; i < art->height() - 20; i += 50, bigDash = (bigDash + 1) % 5) {
        if (drawNet) {
            QPen backup = p.pen();
            p.setPen(Qt::DotLine);
            p.drawLine(QPoint(0, i), QPoint(width(), i));
            p.setPen(backup);
        }

        p.drawLine(QPoint(0, i), QPoint(6 + (5 + art->width() * drawNet) * (bigDash == 0), i));
        p.drawLine(QPoint(art->width() - 6 - 5 * (bigDash == 0), i), QPoint(art->width(), i));
        if (bigDash == 0) {
            QString text = QString::number(backwardCoord(mirr(QPoint(0, i))).y());
            if (mirr(newCoord(0,0)).y() == i)
                text = "0.000";

            drawText(&p, QPoint(5, i + 12), text);
            bigDash = 0;
        }
    }
}

QPoint NativeDrawer::mirr(QPoint p) {
    p.setY(art->height() - p.y());
    return p;
}

void NativeDrawer::drawText(QPainter *p, QPoint point, QString text) {
    if (text == "")
        return;

    QFontMetrics fm(p->font());
    p->fillRect(point.x(), point.y() + 1, fm.width(text) + 2, -11, QBrush(QColor(255, 255, 255, 200)));
    p->drawText(point, text);
}

void NativeDrawer::leaveEvent(QEvent *event) {
    mousePressed = false;
    repaint();
    event->accept();
}

void NativeDrawer::print() {
    QPrintDialog *dialog = new QPrintDialog(this);
    QObject::connect(dialog, SIGNAL(accepted(QPrinter*)), this, SLOT(nativePrint(QPrinter*)));
    dialog->exec();
}

void NativeDrawer::nativePrint(QPrinter *printer) {
    printer->setOrientation(QPrinter::Landscape);
    printer->setColorMode(QPrinter::Color);
    bool tmp = live;
    live = false;
    QImage *tmpArt = art;
    art = new QImage(printer->width() - 5, printer->height(), QImage::Format_ARGB32);
    nativePaint(true);
    live = tmp;

    QPainter painter(printer);
    painter.drawImage(0, 0, *art);
    painter.end();

    delete art;
    art = tmpArt;

    repaint();
}

void NativeDrawer::sourceDetect(int a, int b) {
    // What is it doing here?

    int ray = 0;
    for (int i = 0; i < data.rays; i++)
        if (rayVisibles[i]) {
            ray = i;
            break;
        }

    QString resStr;
    double average = 0;
    for (int k = 0; k < data.channels - 1; k++) {
        QVector<double> res;
        for (int i = a; i < b; i++)
            res.push_back(data.data[module][k][ray][i]);


        std::sort(res.data(), res.data() + res.size());
        double current = (res[res.size() - 5] - res[5]);
        average += current / (data.channels - 1);
        resStr = resStr + " " + QString::number(current, 'f', 2);
    }

    if (Settings::settings()->sourceMode() == RotationMeasure)
        resStr = QString::number(average) + " " + resStr;
    else
        resStr = QString::number(average);

    qApp->clipboard()->setText(resStr);

    resStr = "Ray: " + QString::number(ray + 1) + "\n" + resStr;

    QMessageBox::information(this, "source height", resStr);
}

void NativeDrawer::exportDataToCSV() {
    QString csvFile = QFileDialog::getSaveFileName(this);
    if (csvFile == "")
        return;

    if (!csvFile.endsWith(".csv"))
        csvFile += ".csv";

    QFile file(csvFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Problem", "can't open file " + csvFile + " for writing");
        return;
    }

    char S = ';'; // splitter

    int ray = 0;
    for (int i = 0; i < data.rays; i++)
        if (rayVisibles[i])
            ray = i;

    file.write(QString::asprintf("Name \\ Module %d", module + 1).toUtf8());

    QVector<float*> dataRays;

    if (data.channels == 1) {
        file.write(&S, 1);
        for (int ray = 0; ray < data.rays; ray++) {
            file.write("R" + QString::number(ray + 1).toUtf8() + S);
            dataRays.push_back(data.data[module][0][ray]);
        }
    } else {
        file.write(QString::asprintf(" Ray %d%c", ray + 1, S).toUtf8());
        for (int channel = 0; channel < data.channels; channel++) {
            file.write("C" + QString::number(channel + 1).toUtf8() + S);
            dataRays.push_back(data.data[module][channel][ray]);
        }
    }

    file.write("\n");

    QStringList names = Settings::settings()->getLastHeader()["stairs_names"].split(",");
    for (int i = 0; i < data.npoints; i++) {
        if (names.size() <= 1)
            if (data.time.isValid())
                file.write(StarTime::StarTime(data, i).toUtf8() + S);
            else
                file.write(QString::number(i + 1).toUtf8() + S);
        else
            file.write(names[i].toUtf8() + S);

        for (int j = 0; j < dataRays.size(); j++)
            file.write(QString::number(dataRays[j][i], 'f').toUtf8() + S);

        file.write("\n");
    }

    file.close();

    QMessageBox::information(this, "Success",
                             "Export to " + csvFile + " was completed successfully!\n"
                             "Be sure to use correct decimal separator (, or .)");
}

void NativeDrawer::applyMedianFilter() {
    Data prev = data;
    data.fork();

    if (data.name.endsWith(".pnthr")) {
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QDialog *dialog = new QDialog;
        dialog->setWindowTitle("Long operation pending");

        QVBoxLayout *layout = new QVBoxLayout(dialog);
        layout->addWidget(new QLabel("This will take a while.\nShould I continue?"));
        layout->addWidget(buttons);

        QObject::connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
        QObject::connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));
        int res = dialog->exec();
        delete dialog;
        if (res == QDialog::Rejected)
            return;
    }

    const int rad = 3;
    for (int module = 0; module < data.modules; module++)
        for (int channel = 0; channel < data.channels; channel++)
            for (int ray = 0; ray < data.rays; ray++) {
                Settings::settings()->getProgressBar()->setValue((ray + channel * data.rays + module * data.rays * data.channels) * 100 / (data.rays*data.modules*data.channels));
                for (int i = rad; i < prev.npoints - rad; i++) {
                    QVector<float> tmp;
                    for (int j = -rad; j <= rad; j++)
                        tmp.push_back(prev.data[module][channel][ray][i + j]);

                    std::sort(tmp.begin(), tmp.end());
                    data.data[module][channel][ray][i] = tmp[tmp.size() / 2];
                }
            }

    data.message += "!Modified by median filter!";
    prev.releaseData();
    resetVisibleRectangle();
}
