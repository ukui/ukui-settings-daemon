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

INCLUDEPATH += \
        -I $$PWD/../../common/          \
        -I ukui-settings-daemon/        \

PKGCONFIG += \
            xrandr x11 gtk+-3.0 \
            glib-2.0 mate-desktop-2.0

SOURCES += \
    xrandr-manager.cpp \
    xrandr-plugin.cpp

HEADERS += \
    xrandr-manager.h \
    xrandr_global.h \
    xrandr-plugin.h

xrandr_lib.path  = $${PLUGIN_INSTALL_DIRS}
xrandr_lib.files = $$OUT_PWD/libxrandr.so

INSTALLS += xrandr_lib

