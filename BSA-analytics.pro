#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T18:24:38
#
#-------------------------------------------------

QT       += core gui

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
    customopendialog.cpp

HEADERS  += mainwindow.h \
    drawer.h \
    data.h \
    reader.h \
    nativedrawer.h \
    colorwidget.h \
    controller.h \
    customopendialog.h

FORMS    += mainwindow.ui \
    customopendialog.ui
