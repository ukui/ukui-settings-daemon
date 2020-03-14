QT += dbus
QT -= gui

TEMPLATE = lib

TARGET = xrandr

DEFINES += XRANDR_LIBRARY

CONFIG += c++11 no_keywords plugin link_pkgconfig

PKGCONFIG += \
        gtk+-3.0

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common                           \
        -I ukui-settings-daemon/                        \
        -I /usr/include/glib-2.0/                       \
        -I /usr/lib/glib-2.0/include                    \
        -I /usr/lib/x86_64-linux-gnu/glib-2.0/include/  \
        -I /usr/include/dbus-1.0                        \
        -I /usr/lib/dbus-1.0/include/                   \
        -I /usr/lib/x86_64-linux-gnu/dbus-1.0/include/  \
        -I /usr/include/mate-desktop-2.0/               \
        -I /usr/include/gtk-3.0/                        \
        -I /usr/include/cairo/                          \
        -I /usr/include/pango-1.0/                      \
        -I /usr/include/gtk-3.0/gdk/                    \
        -I /usr/include/gdk-pixbuf-2.0/                 \
        -I /usr/include/atk-1.0/                        \
        -I /usr/include/libnotify/

LIBS += \
        -lglib-2.0          \
        -ldbus-1            \
        -lgobject-2.0       \
        -lgio-2.0           \
        -L /lib64           \
        -ldbus-glib-1       \
        -lmate-desktop-2

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    xrandr-manager.cpp \
    xrandr-plugin.cpp

HEADERS += \
    xrandr-manager.h \
    xrandr_global.h \
    xrandr-plugin.h

DESTDIR = $$PWD/../../library/
