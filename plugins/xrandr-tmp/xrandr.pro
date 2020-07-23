QT += dbus
QT -= gui

TEMPLATE = lib

TARGET = xrandr

DEFINES += XRANDR_LIBRARY

CONFIG += c++11 no_keywords plugin link_pkgconfig

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gtk+-3.0    \
        glib-2.0    \
        gobject-2.0 \
        dbus-1 cairo pango\
        pangocairo \
        gdk-pixbuf-2.0 atk \
        libnotify

INCLUDEPATH += \
        -I $$PWD/../../common/      \
        -I ukui-settings-daemon/    \
        -I /usr/include/mate-desktop-2.0/

LIBS += \
        /usr/lib/x86_64-linux-gnu/libmate-desktop-2.so

SOURCES += \
    xrandr-manager.cpp \
    xrandr-plugin.cpp

HEADERS += \
    xrandr-manager.h \
    xrandr_global.h \
    xrandr-plugin.h

DESTDIR = $$PWD/
xrandr_lib.path = /usr/local/lib/ukui-settings-daemon/
xrandr_lib.files = $$PWD/libxrandr.so 

INSTALLS += xrandr_lib

