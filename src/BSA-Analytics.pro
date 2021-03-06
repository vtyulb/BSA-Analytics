#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T18:24:38
#
#-------------------------------------------------

QT       += core gui printsupport network svg

CONFIG += console c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BSA-Analytics
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp\
    drawer.cpp \
    reader.cpp \
    nativedrawer.cpp \
    colorwidget.cpp \
    controller.cpp \
    customopendialog.cpp \
    startime.cpp \
    pulsarsearcher.cpp \
    pulsarprocess.cpp \
    pulsarworker.cpp \
    calculationpool.cpp \
    pulsarreader.cpp \
    pulsarlist.cpp \
    settings.cpp \
    analytics.cpp \
    wavplayer.cpp \
    filecompressor.cpp \
    precisesearchgui.cpp \
    knownnoise.cpp \
    spectredrawer.cpp \
    nativespectredrawer.cpp \
    precisetiming.cpp \
    preciseperioddetecter.cpp \
    flowdetecter.cpp \
    datadumper.cpp \
    flowingwindow.cpp \
    fourier.cpp \
    fouriersearch.cpp \
    updater.cpp \
    datagenerator.cpp \
    datachecker.cpp \
    transientperiod.cpp \
    datarenamer.cpp \
    transientdetalizator.cpp \
    crosscorrelation.cpp \
    massprocessor.cpp

HEADERS  += mainwindow.h \
    drawer.h \
    data.h \
    reader.h \
    nativedrawer.h \
    colorwidget.h \
    controller.h \
    customopendialog.h \
    startime.h \
    pulsarsearcher.h \
    pulsarprocess.h \
    pulsar.h \
    pulsarworker.h \
    calculationpool.h \
    pulsarreader.h \
    pulsarlist.h \
    settings.h \
    analytics.h \
    wavplayer.h \
    filecompressor.h \
    knownpulsar.h \
    precisesearchgui.h \
    knownnoise.h \
    spectredrawer.h \
    nativespectredrawer.h \
    precisetiming.h \
    preciseperioddetecter.h \
    flowdetecter.h \
    datadumper.h \
    flowingwindow.h \
    fourier.h \
    fouriersearch.h \
    updater.h \
    datagenerator.h \
    datachecker.h \
    transientperiod.h \
    datarenamer.h \
    transientdetalizator.h \
    crosscorrelation.h \
    massprocessor.h

FORMS    += mainwindow.ui \
    customopendialog.ui \
    analytics.ui \
    precisesearchgui.ui \
    knownnoise.ui \
    spectre.ui \
    precisetiming.ui \
    transientperiod.ui

win32:RC_FILE = icon.rc

RESOURCES = resources.qrc

GIT_VERSION = $$system(git describe --always --tags)

DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"
