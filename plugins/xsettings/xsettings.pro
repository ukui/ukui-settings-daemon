#-------------------------------------------------
#
# Project created by QtCreator 2020-03-10T16:29:24
#
#-------------------------------------------------

QT       += dbus

QT       -= gui

TARGET = xsettings
TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

#DEFINES += XSETTINGS_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/../../common/common.pri)

PKGCONFIG += glib-2.0 \
             gio-2.0

INCLUDEPATH += \
        -I $$PWD/../../common

SOURCES += \
        xsettings.cpp \
    xsettings-manager.cpp \
    ixsettings-manager.cpp \
    xsettings-common.c

HEADERS += \
        xsettings.h \
        xsettings_global.h \ 
    ixsettings-manager.h \
    xsettings-common.h \
    xsettings-manager.h


DESTDIR = $$PWD/../../library/
