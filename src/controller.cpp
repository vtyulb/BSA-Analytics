#include <controller.h>
#include <startime.h>
#include <settings.h>
#include <pulsar.h>

#include <QVBoxLayout>
#include <QDebug>

Controller::Controller(QWidget *parent) :
    QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    QVBoxLayout *layout = new QVBoxLayout(this);

    coords = new QLabel(this);
    rays = new QLabel(this);
    points = new QLabel(this);
    channels = new QLabel(this);
    modules = new QLabel(this);
    sky = new QLabel(this);
    fileName = new QLabel(this);
    nativeXCoord = new QLabel(this);

    layout->addWidget(rays);
    layout->addWidget(channels);
    layout->addWidget(modules);
    layout->addWidget(points);
    layout->addWidget(sky);
    layout->addWidget(coords);
    layout->addWidget(nativeXCoord);
    layout->addWidget(fileName);
    layout->addStretch(5);
}

void Controller::setCoords(QPointF p) {
    double realSeconds;
    QString starTime = StarTime::StarTime(data, p.x(), &realSeconds);
    starTime += "." + QString::number(int(realSeconds * 100) / 10 % 10) + QString::number(int(realSeconds * 100) % 10);
    coords->setText(QString("X: %1; Y: %2").arg(starTime, QString::number(p.y())));
    nativeXCoord->setText(QString("X: %1").arg(p.x()));
    if (Settings::settings()->fourierAnalytics())
        nativeXCoord->setText(QString("X: %1; p=%2s").arg(QString::number(p.x(), 'f', 1), QString::number(2048.0 / p.x() * 0.0999424, 'f', 5)));
}

void Controller::setRays(int r) {
    rays->setText(QString("%1 rays detected").arg(QString::number(r)));
}

void Controller::setPoints(int p) {
    points->setText(QString("%1 points on ray").arg(QString::number(p)));
}

void Controller::setChannels(int c) {
    channels->setText(QString("%1 channels").arg(QString::number(c)));
}

void Controller::setModules(int m) {
    modules->setText(QString("%1 modules").arg(QString::number(m)));
}

void Controller::resetSky(Data newData, int module, QVector<bool> rays) {
    data = newData;
    if (Settings::settings()->fourierAnalytics()) {
        setFileName(data.previousLifeName);
        sky->hide();
    } else {
        int first = 1;
        int last = 1;
        for (int i = 0; i < rays.size(); i++)
            if (rays[i]) {
                first = i;
                break;
            }

        for (int i = rays.size() - 1; i >= 0; i--)
            if (rays[i]) {
                last = i;
                break;
            }

        QString res = getPulsarJName(module, (first + last) / 2 + 1, QTime(0,0)).right(5);
        sky->setText(res.left(3) + "°" + res.right(2) + "'");
    }
}

void Controller::setFileName(QString s) {
    if (s != "")
        fileName->setText(s);
}
