TEMPLATE = app

QT += core gui
CONFIG += console c++11
CONFIG -= app_bundle
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += \
        -I /usr/include/glib-2.0/\
        -I /usr/lib/glib-2.0/include\
        -I /usr/lib/x86_64-linux-gnu/glib-2.0/include/\
        -I /usr/include/dbus-1.0 \
        -I /usr/lib/dbus-1.0/include/\
        -I /usr/lib/x86_64-linux-gnu/dbus-1.0/include/\
        -I /usr/include/mate-desktop-2.0/

LIBS += \
        -lglib-2.0\
        -ldbus-1\
        -lgobject-2.0\
        -lgio-2.0\
        -L /lib64\
        -ldbus-glib-1\
        -lmate-desktop-2

SOURCES += \
        clib_syslog.c\
        main.cpp \
        ukuisettingsmanager.cpp \
        ukuisettingsmodule.cpp \
        ukuisettingsplugin.cpp \
        ukuisettingsplugininfo.cpp

HEADERS += \
        clib_syslog.h\
        global.h \
        ukuisettingsmanager.h \
        ukuisettingsmodule.h \
        ukuisettingsplugin.h \
        ukuisettingsplugininfo.h
