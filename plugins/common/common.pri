#-------------------------------------------------
#
# Project created by QtCreator 2020-03-16T20:31:34
#
#-------------------------------------------------
QT       += gui
CONFIG += c++11 no_keywords link_pkgconfig plugin

PKGCONFIG += glib-2.0 atk gio-2.0 gdk-3.0 gtk+-3.0 xi

INCLUDEPATH += \
        -I $$PWD \
        -I $$PWD/../..

SOURCES += \
        $$PWD/eggaccelerators.c \
        $$PWD/ukui-input-helper.c \
        $$PWD/ukui-keygrab.cpp

HEADERS += \
        $$PWD/eggaccelerators.h \
        $$PWD/ukui-input-helper.h \
        $$PWD/ukui-keygrab.h
