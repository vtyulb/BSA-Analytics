#include "drawer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QTimer>
#include <colorwidget.h>

const char* colorNames[16] = {"FF0000",
                              "00FF00",
                              "0000FF",
                              "000000",
                              "FFFF00",
                              "00FFFF",
                              "FF00FF",
                              "800000",
                              "008000",
                              "000080",
                              "808000",
                              "800080",
                              "D0A080",
                              "00D0D0",
                              "D000C0",
                              "00D000"};

Drawer::Drawer(const Data data, QWidget *parent) :
    QWidget(parent),
    numberChannels(data[0].size()),
    numberModules(data.size())
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    controlFrame = new QFrame(this);
    drawer = new NativeDrawer(data);
    layout->addWidget(drawer);
    layout->addWidget(controlFrame);

    QVBoxLayout *l = new QVBoxLayout(controlFrame);
    for (int i = 0; i < data[0][0][0].size(); i++) {
        checkBoxes.push_back(new QCheckBox(QString("ray %1%2").arg(QString::number((i + 1)/10), QString::number((i + 1)%10)), this));
        checkBoxes[i]->setChecked(true);
        QObject::connect(checkBoxes[i], SIGNAL(clicked()), this, SLOT(checkBoxStateChanged()));

        QWidget *widget = new QWidget(this);
        colors.push_back(new QLineEdit(this));
        colors[i]->setText(colorNames[i]);

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

    QFrame *hline = new QFrame(this);
    hline->setFrameStyle(QFrame::HLine);
    l->addWidget(hline);

    channel = new QSpinBox(this);
    channel->setMinimum(1);
    channel->setMaximum(data[0].size());
    channel->setValue(data[0].size());
    QObject::connect(channel, SIGNAL(valueChanged(int)), this, SLOT(channelChanged(int)));

    QWidget *channelWidget = new QWidget(this);
    QHBoxLayout *channelWidgetLayout = new QHBoxLayout(channelWidget);
    QLabel *channelLabel = new QLabel("Channel", this);
    channelWidgetLayout->addWidget(channelLabel);
    channelWidgetLayout->addWidget(channel);

    l->addWidget(channelWidget);

    QFrame *moduleFrame = new QFrame(this);
    moduleFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    QVBoxLayout *modulesLayout = new QVBoxLayout(moduleFrame);
    modulesLayout->setContentsMargins(1, 1, 1, 1);
    QButtonGroup *randomGroup = new QButtonGroup(this);
    for (int i = 0; i < data.size(); i++) {
        modules.push_back(new QRadioButton(QString("module %1").arg(QString::number(i + 1))));
        modulesLayout->addWidget(modules[i]);
        randomGroup->addButton(modules[i]);

        QObject::connect(modules[i], SIGNAL(clicked()), this, SLOT(moduleChanged()));
    }

    modules[0]->setChecked(true);
    l->addWidget(moduleFrame);

    l->addStretch(10);
    resetButton = new QPushButton(this);
    resetButton->setText("reset");
    l->addWidget(resetButton);
    enableAll = new QPushButton(this);
    enableAll->setText("Enable all rays");
    l->addWidget(enableAll);
    disableAll = new QPushButton(this);
    disableAll->setText("Disable all rays");
    l->addWidget(disableAll);
    drawButton = new QCommandLinkButton(this);
    drawButton->setText("Draw");
    l->addWidget(drawButton);

    QObject::connect(enableAll, SIGNAL(clicked()), this, SLOT(enableAllRays()));
    QObject::connect(disableAll, SIGNAL(clicked()), this, SLOT(disableAllRays()));
    QObject::connect(resetButton, SIGNAL(clicked()), drawer, SLOT(resetVisibleRectangle()));
    QObject::connect(drawButton, SIGNAL(clicked()), this, SLOT(draw()));

    controlFrame->setMaximumWidth(188);
    controlFrame->setMinimumWidth(188);

    controller = new Controller(this);
    controller->setModules(data.size());
    controller->setChannels(data[0].size());
    controller->setPoints(data[0][0].size());
    controller->setRays(data[0][0][0].size());
    l->addWidget(controller);

    QObject::connect(drawer, SIGNAL(coordsChanged(QPoint)), controller, SLOT(setCoords(QPoint)));

    rays = data[0][0][0].size();
    show();

    QTimer::singleShot(10, this, SLOT(draw()));
}

Drawer::~Drawer() {
//    delete drawer;
//    delete controlFrame;
//    delete resetButton;
//    delete disableAll;
//    delete enableAll;
//    delete drawButton;
//    delete controller;
}

void Drawer::checkBoxStateChanged() {
    QVector<bool> v;
    for (int i = 0; i < rays; i++)
        v.push_back(checkBoxes[i]->isChecked());

    drawer->setRayVisibles(v);
}

void Drawer::channelChanged(int channel) {
    drawer->channel = channel - 1;
    drawer->resetVisibleRectangle();
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
        v.push_back(colors[i]->text());

    drawer->setColors(v);
    drawer->allowDrawing = true;
    drawer->nativePaint();
}

void Drawer::moduleChanged() {
    for (int i = 0; i < numberModules; i++)
        if (modules[i]->isChecked())
            drawer->module = i;

    drawer->resetVisibleRectangle();
}
