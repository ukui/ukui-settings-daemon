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
        gsettings-qt


INCLUDEPATH += \
        -I $$PWD/../../common       \
        -I $$PWD/../common          \
        -I ukui-settings-daemon/

SOURCES += \
    mouse-manager.cpp \
    mouse-plugin.cpp \

HEADERS += \
    mouse-manager.h \
    mouse-plugin.h \


DESTDIR = $$PWD/

mouse_lib.path = $${PLUGIN_INSTALL_DIRS}
mouse_lib.files = $$PWD/libmouse.so

INSTALLS += mouse_lib
