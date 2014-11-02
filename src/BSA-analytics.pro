#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T18:24:38
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BSA-analytics
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
    writer.cpp

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
    writer.h \
    pulsar.h

FORMS    += mainwindow.ui \
    customopendialog.ui

win32:RC_FILE = icon.rc
