#-------------------------------------------------
#
# Project created by QtCreator 2020-03-16T20:31:34
#
#-------------------------------------------------

QT       -= gui

TARGET = common
TEMPLATE = lib
CONFIG += c++11 no_keywords link_pkgconfig plugin
DEFINES += COMMON_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG +=     glib-2.0 \
    atk    \
    gio-2.0 \
    gdk-3.0 

INCLUDEPATH += \
        -I $$PWD \
    -I $$PWD/../..




SOURCES += \
        eggaccelerators.c \
        ukui-osd-window.cpp \
        ukui-input-helper.c \
        ukui-keygrab.c

HEADERS += \
        eggaccelerators.h \
        ukui-osd-window.h \
        common_global.h \
        usd-input-helper.h \
        ukui-keygrab.h

DESTDIR = $$PWD/

common_lib.path = /usr/local/lib/ukui-settings-daemon/
common_lib.files = $$PWD/libcommon.so

INSTALLS += common_lib
