QT       += dbus
QT       -= gui
TEMPLATE = lib
TARGET = xsettings

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG +=\
    glib-2.0 \
    gio-2.0 \
    gdk-3.0 \
    atk

INCLUDEPATH += \
    -I $$PWD/../../common

SOURCES += \
    xsettings-manager.cpp \
    ukui-xsettings-manager.cpp \
    ukui-xft-settings.cpp \
    xsettings-plugin.cpp \
    xsettings-common.c \
    fontconfig-monitor.c

HEADERS += \
    xsettings-manager.h \
    ukui-xsettings-manager.h \
    ukui-xft-settings.h \
    xsettings-const.h \
    xsettings-plugin.h  \
    xsettings-common.h  \
    fontconfig-monitor.h


DESTDIR = $$PWD
xsettings_lib.path = /usr/local/lib/ukui-settings-daemon/
xsettings_lib.files += $$PWD/libxsettings.so

INSTALLS += xsettings_lib
