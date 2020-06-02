#-------------------------------------------------
#
# Project created by QtCreator 2020-03-13T09:50:48
#
#-------------------------------------------------

QT       -= gui
QT       += dbus

TARGET = xrdb
TEMPLATE = lib

DEFINES += XRDB_LIBRARY

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

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


PKGCONFIG +=\
    gtk+-3.0 \
    gsettings-qt \
    atk    \
    mate-desktop-2.0

INCLUDEPATH += \
    -I $$PWD/../../common


SOURCES += \
    xrdb.cpp \
    ukui-xrdb-manager.cpp

HEADERS += \
    xrdb.h \
    ukui-xrdb-manager.h \
    ixrdb-manager.h

DESTDIR = $$PWD

xrdb_lib.path = /usr/local/lib/ukui-settings-daemon/
xrdb_lib.files += $$PWD/libxrdb.so \

INSTALLS += xrdb_lib

