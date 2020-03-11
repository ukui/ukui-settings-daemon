QT       += dbus

QT       -= gui
TEMPLATE = lib
TARGET = xsettings

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

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
    ixsettings-manager.h \
    xsettings-common.h \
    xsettings-manager.h


DESTDIR = $$PWD/../../library/
