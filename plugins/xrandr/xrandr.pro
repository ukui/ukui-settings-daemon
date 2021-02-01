#-------------------------------------------------
#
# Project created by QtCreator 2020-05-10T09:30:00
#
#-------------------------------------------------
QT += gui
QT += core xml widgets x11extras dbus

TEMPLATE = lib
DEFINES += XRANDR_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin xrandr
CONFIG += app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        Qt5Sensors  \
        kscreen2    \
        x11         \
        xi          \
        gudev-1.0

SOURCES += \
    xrandr-adaptor.cpp \
    xrandr-config.cpp \
    xrandr-dbus.cpp \
    xrandr-manager.cpp \
    xrandr-output.cpp \
    xrandr-plugin.cpp

HEADERS += \
    xrandr-adaptor.h \
    xrandr-config.h \
    xrandr-dbus.h \
    xrandr-manager.h \
    xrandr-output.h \
    xrandr-plugin.h

xrandr_lib.path  = $${PLUGIN_INSTALL_DIRS}
xrandr_lib.files = $$OUT_PWD/libxrandr.so

INSTALLS += xrandr_lib

