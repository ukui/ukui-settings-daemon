#-------------------------------------------------
#
# Project created by QtCreator 2020-06-24T09:30:00
#
#-------------------------------------------------
QT -= gui
QT += dbus
TEMPLATE = lib
TARGET = mpris

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
    -I $$PWD/../../common/

PKGCONFIG += \
    glib-2.0 \
    gio-2.0

SOURCES += \
    mprismanager.cpp \
    mprisplugin.cpp

HEADERS += \
    mprismanager.h \
    mprisplugin.h

DESTDIR = $$PWD/

mpris_lib.path = /usr/local/lib/ukui-settings-daemon/
mpris_lib.files = $$PWD/libmpris.so

INSTALLS += mpris_lib
