#-------------------------------------------------
#
# Project created by QtCreator 2012-07-24T22:35:51
#
#-------------------------------------------------

QT       += core gui phonon network xml

TARGET = amcpp
TEMPLATE = app

LIBS += -lqca

SOURCES += main.cpp\
        amcpp.cpp \
    configdialog.cpp

HEADERS  += amcpp.h \
    configdialog.h

FORMS    += amcpp.ui \
    configdialog.ui
