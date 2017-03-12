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
    stairFileName = new QLabel(this);

    fileName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    layout->addWidget(rays);
    layout->addWidget(channels);
    layout->addWidget(modules);
    layout->addWidget(points);
    layout->addWidget(sky);
    layout->addWidget(coords);
    layout->addWidget(nativeXCoord);
    layout->addWidget(fileName);
    layout->addWidget(stairFileName);
    layout->addStretch(5);

    stairFileName->hide();
}

void Controller::setCoords(QPointF p) {
    double realSeconds;
    QString starTime = StarTime::StarTime(data, p.x(), &realSeconds);
    if (starTime != "") {
        starTime += "." + QString::number(int(realSeconds * 100) / 10 % 10) + QString::number(int(realSeconds * 100) % 10);
        coords->setText(QString("X: %1; Y: %2").arg(starTime, QString::number(p.y())));
    } else
        coords->setText(QString("Y: %1").arg(p.y()));

    nativeXCoord->setText(QString("X: %1").arg(p.x()));
    if (Settings::settings()->fourierAnalytics())
        nativeXCoord->setText(QString("X: %1; p=%2s").arg(QString::number(p.x(), 'f', 1),
                              QString::number(Settings::settings()->getFourierSpectreSize() * 2 / (p.x() + 1) * Settings::settings()->getFourierStepConstant(), 'f', 5)));

    if (Settings::settings()->getLastHeader().contains("stairs_names"))
        nativeXCoord->setText(Settings::settings()->getLastHeader()["stairs_names"].split(",").value(int(p.x())));

    if (Settings::settings()->sourceMode()) {
        stairFileName->setText("Stair from " + Settings::settings()->stairFileName());
        stairFileName->show();
    } else
        stairFileName->hide();
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
        setFileName(data.previousLifeName.replace("file ", "").replace("from", "<-"));
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

        QString res = getPulsarJName(module + 1, (first + last) / 2 + 1, QTime(0,0)).right(5);
        sky->setText(res.left(3) + "°" + res.right(2) + "'");
    }
}

void Controller::setFileName(QString s) {
    if (s != "")
        fileName->setText(s);
}
