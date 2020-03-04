TEMPLATE = app
QT += core gui
CONFIG += console c++11
CONFIG -= app_bundle
#CONFIG -= qt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += \
        -I ukui-settings-daemon/ \
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
        ukui-settings-daemon/clib_syslog.c\
        ukui-settings-daemon/main.cpp \
        ukui-settings-daemon/ukuisettingsmanager.cpp \
        ukui-settings-daemon/ukuisettingsmodule.cpp \
        ukui-settings-daemon/ukuisettingsplugin.cpp \
        ukui-settings-daemon/ukuisettingsplugininfo.cpp \
        ukui-settings-daemon/ukuisettingsprofile.cpp

HEADERS += \
        ukui-settings-daemon/clib_syslog.h\
        ukui-settings-daemon/ukuisettingsmanager.h \
        ukui-settings-daemon/ukuisettingsmodule.h \
        ukui-settings-daemon/ukuisettingsplugin.h \
        ukui-settings-daemon/ukuisettingsplugininfo.h \
        ukui-settings-daemon/ukuisettingsprofile.h
