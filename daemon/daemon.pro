TEMPLATE = app

QT += core gui
CONFIG += c++11
CONFIG -= app_bundle

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += \
        -I $$PWD/../common/\
        -I /usr/include/glib-2.0/\
        -I /usr/lib/glib-2.0/include\
        -I /usr/lib/x86_64-linux-gnu/glib-2.0/include/\
        -I /usr/include/dbus-1.0 \
        -I /usr/lib/dbus-1.0/include/\
        -I /usr/lib/x86_64-linux-gnu/dbus-1.0/include/\
        -I /usr/include/mate-desktop-2.0/

LIBS += \
        -L /lib64\
        -lglib-2.0\
        -ldbus-1\
        -lgobject-2.0\
        -lgio-2.0\
        -ldbus-glib-1\
        -lmate-desktop-2\
        -lgmodule-2.0\
        -lbaseplugin

SOURCES += \
        $$PWD/main.cpp \
        $$PWD/ukuisettingsmodule.cpp \
        $$PWD/ukuisettingsmanager.cpp \
        $$PWD/ukuisettingsplugininfo.cpp \
#        a.cpp

HEADERS += \
        $$PWD/global.h \
        $$PWD/ukuisettingsmodule.h \
        $$PWD/ukuisettingsmanager.h \
        $$PWD/ukuisettingsplugininfo.h
