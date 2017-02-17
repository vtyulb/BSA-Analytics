#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T18:24:38
#
#-------------------------------------------------

QT       += core gui printsupport network

CONFIG += console

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
    knownpulsarsgui.cpp \
    knownnoise.cpp \
    spectredrawer.cpp \
    nativespectredrawer.cpp \
    precisetiming.cpp \
    preciseperioddetecter.cpp \
    flowdetecter.cpp \
    filesummator.cpp \
    datadumper.cpp \
    flowingwindow.cpp \
    fourier.cpp \
    fouriersearch.cpp \
    updater.cpp

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
    knownpulsarsgui.h \
    knownnoise.h \
    spectredrawer.h \
    nativespectredrawer.h \
    precisetiming.h \
    preciseperioddetecter.h \
    flowdetecter.h \
    filesummator.h \
    datadumper.h \
    flowingwindow.h \
    fourier.h \
    fouriersearch.h \
    updater.h

FORMS    += mainwindow.ui \
    customopendialog.ui \
    pulsarlist.ui \
    analytics.ui \
    precisesearchgui.ui \
    precisepacket.ui \
    knownpulsarsgui.ui \
    knownnoise.ui \
    spectre.ui \
    precisetiming.ui \
    flowdetecter.ui

win32:RC_FILE = icon.rc
