#-------------------------------------------------
#
# Project created by QtCreator 2020-04-10T09:30:00
#
#-------------------------------------------------
QT += gui
QT += core widgets x11extras
TARGET = mouse
TEMPLATE = lib
DEFINES += MOUSE_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin
CONFIG += app_bunale

DEFINES += QT_DEPRECATED_WARNINGS



include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gtk+-3.0 \
        glib-2.0  \
        gsettings-qt \
        xi


INCLUDEPATH += \
        -I ukui-settings-daemon/

SOURCES += \
    mouse-manager.cpp \
    mouse-plugin.cpp \

HEADERS += \
    mouse-manager.h \
    mouse-plugin.h \

mouse_lib.path = $${PLUGIN_INSTALL_DIRS}
mouse_lib.files = $$OUT_PWD/libmouse.so

touchpad.path = /usr/bin/
touchpad.files = $$PWD/touchpad-state

udev.path = /lib/udev/rules.d/
udev.files = $$PWD/01-touchpad-state-onmouse.rules

INSTALLS += mouse_lib touchpad udev
