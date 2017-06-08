#include "drawer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QTimer>
#include <QScrollArea>
#include <pulsarprocess.h>
#include <colorwidget.h>

const char* colorNames[17] = {"FF0000",
                              "00FF00",
                              "0000FF",
                              "000000",
                              "FFAF00",
                              "00A0A0",
                              "FF00FF",
                              "800000",
                              "008000",
                              "000080",
                              "808000",
                              "800080",
                              "D0A080",
                              "00D0D0",
                              "D000C0",
                              "00D000",
                              "WTF???"};

Drawer::Drawer(const Data &data, QWidget *parent) :
    QWidget(parent),
    rays(data.rays),
    numberChannels(data.channels),
    numberModules(data.modules)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    controlFrame = new QFrame(this);
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(controlFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidgetResizable(true);

    drawer = new NativeDrawer(data, this);
    layout->addWidget(drawer);
    layout->addWidget(scrollArea);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    scrollArea->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);


    QVBoxLayout *l = new QVBoxLayout(controlFrame);
    for (int i = 0; i < std::min(data.rays, 16); i++) {
        checkBoxes.push_back(new QCheckBox(QString("Ray %1%2").arg(QString::number((i + 1)/10), QString::number((i + 1)%10)), this));
        checkBoxes[i]->setChecked(true);
        QObject::connect(checkBoxes[i], SIGNAL(clicked()), this, SLOT(checkBoxStateChanged()));

        QWidget *widget = new QWidget(this);
        colors.push_back(new QLineEdit(this));
        colors[i]->setText(colorNames[i]);
        colors[i]->setMaximumWidth(QFontMetrics(colors[i]->font()).width("FFFFFF") + 80);

        ColorWidget *w = new ColorWidget(colors[i], this);
        w->setMinimumWidth(20);
        w->setMaximumWidth(20);

        QHBoxLayout *lay = new QHBoxLayout(widget);
        lay->addWidget(checkBoxes[i]);
        lay->addWidget(w);
        lay->addWidget(colors[i]);
        lay->setContentsMargins(1, 1, 1, 1);
        l->addWidget(widget);
    }

    QWidget *enableAndDisableButtons = new QWidget;
    QHBoxLayout *enableAndDisableButtonsLayout = new QHBoxLayout(enableAndDisableButtons);
    disableAll = new QPushButton(this);
    disableAll->setText("Disable all");
    enableAndDisableButtonsLayout->addWidget(disableAll);
    enableAll = new QPushButton(this);
    enableAll->setText("Enable all");
    enableAndDisableButtonsLayout->addWidget(enableAll);
    enableAndDisableButtonsLayout->setContentsMargins(0, 0, 0, 0);
    l->addWidget(enableAndDisableButtons);

    if (data.channels > 1) {
        channel = new QSpinBox(this);
        channel->setMinimum(1);
        channel->setMaximum(data.channels);
        QObject::connect(channel, SIGNAL(valueChanged(int)), this, SLOT(channelChanged(int)));
        channel->setValue(data.channels);

        QFrame *channelFrame = new QFrame(this);
        channelFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        QHBoxLayout *channelFrameLayout = new QHBoxLayout(channelFrame);
        QLabel *channelLabel = new QLabel("Channel", this);
        channelFrameLayout->addWidget(channelLabel);
        channelFrameLayout->addWidget(channel);

        l->addWidget(channelFrame);
    }

    if (data.modules > 1) {
        QFrame *moduleFrame = new QFrame(this);
        moduleFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        QVBoxLayout *modulesLayout = new QVBoxLayout(moduleFrame);
        modulesLayout->setContentsMargins(1, 1, 1, 1);
        QButtonGroup *randomGroup = new QButtonGroup(this);
        for (int i = 0; i < data.modules; i++) {
            modules.push_back(new QRadioButton(QString("Module %1").arg(QString::number(i + 1)), this));
            modulesLayout->addWidget(modules[i]);
            randomGroup->addButton(modules[i]);

            QObject::connect(modules[i], SIGNAL(clicked()), this, SLOT(moduleChanged()));
        }

        modules[0]->setChecked(true);
        l->addWidget(moduleFrame);
    }

    delta = new QDoubleSpinBox(this);
    delta->setValue(data.delta_lucha);
    QObject::connect(delta, SIGNAL(valueChanged(double)), this, SLOT(deltaChanged(double)));
    QFrame *deltaFrame = new QFrame(this);
    deltaFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    QHBoxLayout *deltaLayout = new QHBoxLayout(deltaFrame);
    QLabel *deltaLabel = new QLabel(this);
    deltaLabel->setText("delta");
    deltaLayout->addWidget(deltaLabel);
    deltaLayout->addWidget(delta);
    deltaFrame->hide();

    l->addWidget(deltaFrame);

    l->addStretch(10);

    QWidget *resetAndDraw = new QWidget;
    QHBoxLayout *resetAndDrawLayout = new QHBoxLayout(resetAndDraw);
    drawButton = new QPushButton(this);
    drawButton->setText("Draw");
    resetAndDrawLayout->addWidget(drawButton);
    resetButton = new QPushButton(this);
    resetButton->setText("Reset");
    resetAndDrawLayout->addWidget(resetButton);
    resetAndDrawLayout->setContentsMargins(0, 0, 0, 0);
    l->addWidget(resetAndDraw);

    QObject::connect(enableAll, SIGNAL(clicked()), this, SLOT(enableAllRays()));
    QObject::connect(disableAll, SIGNAL(clicked()), this, SLOT(disableAllRays()));
    QObject::connect(resetButton, SIGNAL(clicked()), drawer, SLOT(resetVisibleRectangle()));
    QObject::connect(drawButton, SIGNAL(clicked()), this, SLOT(draw()));

//    controlFrame->setMaximumWidth(188);
//    controlFrame->setMinimumWidth(188);

    controller = new Controller(this);
    setControllerData(data);
    l->addWidget(controller);

    setContentsMargins(0, 0, 0, 0);

    QObject::connect(drawer, SIGNAL(coordsChanged(QPointF)), controller, SLOT(setCoords(QPointF)));

    show();

    QTimer::singleShot(10, this, SLOT(draw()));
    QTimer *timer = new QTimer(this);
    timer->setInterval(5000);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timeToDie()));
    timer->start();

    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));

    deltaChanged(delta->value());
}

void Drawer::setControllerData(const Data &data) {
    controller->setModules(data.modules);
    controller->setChannels(data.channels);
    controller->setPoints(data.npoints);
    controller->setRays(data.rays);
    controller->setFileName(data.name);
}

void Drawer::pushNewData(const Data &data) {
    controller->resetSky(data);
    setControllerData(data);

    drawer->data.releaseData();
    drawer->setData(data);
    drawer->resetVisibleRectangle();
}

void Drawer::checkBoxStateChanged() {
    raysEnabled.clear();
    for (int i = 0; i < rays; i++)
        raysEnabled.push_back(checkBoxes[i]->isChecked());

    drawer->setRayVisibles(raysEnabled);
    controller->resetSky(drawer->data, drawer->module, raysEnabled);
}

void Drawer::channelChanged(int channel) {
    drawer->channel = channel - 1;
    drawer->resetVisibleRectangle(true, false);
}

void Drawer::enableAllRays() {
    for (int i = 0; i < rays; i++)
        checkBoxes[i]->setChecked(true);

    checkBoxStateChanged();
}

void Drawer::disableAllRays() {
    for (int i = 0; i < rays; i++)
        checkBoxes[i]->setChecked(false);

    checkBoxStateChanged();
}

void Drawer::saveFile(QString file) {
    drawer->saveFile(file);
}

void Drawer::draw() {
    QVector<QString> v;
    for (int i = 0; i < rays; i++)
        v.push_back(colors[i % 16]->text());

    drawer->setColors(v);
    drawer->allowDrawing = true;
    drawer->nativePaint();

    /*
    static int one = 0;
    qDebug() << "debugging pulsar feauture enabled!!! Don't forget to delete" << one;
    if (one++ < 2)
        return;

    PulsarProcess *p = new PulsarProcess("wrong name :-)", "wrong path", this);
    p->data = drawer->data;
    p->start();*/
}

void Drawer::moduleChanged() {
    for (int i = 0; i < numberModules; i++)
        if (modules[i]->isChecked())
            drawer->module = i;

    controller->resetSky(drawer->data, drawer->module, raysEnabled);
    drawer->resetVisibleRectangle(true, true);
}

void Drawer::keyPressEvent(QKeyEvent *event) {
    if (event->text() == "f")
        if (parent() == NULL) {
            if (this->isFullScreen())
                showNormal();
            else
                showFullScreen();

            event->accept();
        }
}

void Drawer::timeToDie() {
    if (!isVisible())
        deleteLater();
}

void Drawer::deltaChanged(double d) {
    drawer->data.delta_lucha = d;
    controller->resetSky(drawer->data, drawer->module, raysEnabled);
    drawer->drawAxes();
    drawer->repaint();
}
