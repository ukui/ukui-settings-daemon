#-------------------------------------------------
#
# Project created by QtCreator 2020-03-16T20:31:34
#
#-------------------------------------------------

QT       += gui

TARGET = common
TEMPLATE = lib
CONFIG += c++11 no_keywords link_pkgconfig plugin
DEFINES += COMMON_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS HAVE_X11_EXTENSIONS_XKB_H

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
        ukui-input-helper.c \
        ukui-keygrab.cpp

HEADERS += \
        eggaccelerators.h \
        common_global.h \
        ukui-input-helper.h \
        ukui-keygrab.h

common_lib.path = $${PLUGIN_INSTALL_DIRS}
common_lib.files = $$OUT_PWD/libcommon.so

INSTALLS += common_lib
