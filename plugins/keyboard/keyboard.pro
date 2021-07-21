#-------------------------------------------------
#
# Project created by QtCreator 2020-04-20T09:30:00
#
#-------------------------------------------------
QT += gui
QT += core widgets x11extras
TARGET = keyboard
TEMPLATE = lib
DEFINES += KEYBOARD_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin

DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"keyboard\\\"

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gtk+-3.0 \
        glib-2.0  harfbuzz  gmodule-2.0  \
        libxklavier gobject-2.0 gio-2.0 \
        cairo cairo-gobject gsettings-qt

INCLUDEPATH += \
        -I ukui-settings-daemon/    \
        -I /usr/include/libmatekbd

LIBS += \
        -L/usr/lib/*-linux-gnu -lmatekbdui -lmatekbd \
        -Wl,--export-dynamic -lgmodule-2.0 \
        -pthread -lgdk-3 -lpangocairo-1.0 \
        -lpango-1.0 -lharfbuzz -lgdk_pixbuf-2.0 \
        -lcairo-gobject -lcairo -lgio-2.0 -lgobject-2.0

SOURCES += \
    keyboard-manager.cpp \
    keyboard-plugin.cpp \
    keyboard-xkb.cpp

HEADERS += \
    keyboard-manager.h \
    keyboard-xkb.h \
    keyboard-plugin.h

keyboard_lib.path = $${PLUGIN_INSTALL_DIRS}
keyboard_lib.files = $$OUT_PWD/libkeyboard.so

INSTALLS += keyboard_lib
