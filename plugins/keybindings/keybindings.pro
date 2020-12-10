#-------------------------------------------------
#
# Project created by QtCreator 2020-05-24T09:30:00
#
#-------------------------------------------------
QT -= gui
QT += core widgets x11extras
TARGET = keybindings
TEMPLATE = lib
DEFINES += KEYBINDINGS_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gtk+-3.0    \
        glib-2.0    \
        gobject-2.0 \
        gio-2.0     \
        cairo       \
        gsettings-qt\
        dconf

SOURCES += \
    dconf-util.c \
    keybindings-manager.cpp \
    keybindings-plugin.cpp

HEADERS += \
    dconf-util.h \
    keybindings-manager.h \
    keybindings-plugin.h


keybindings_lib.path = $${PLUGIN_INSTALL_DIRS}
keybindings_lib.files = $$OUT_PWD/libkeybindings.so

INSTALLS += keybindings_lib

