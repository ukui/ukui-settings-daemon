TEMPLATE = app
TARGET = ukui-settings-daemon

QT += core gui dbus
CONFIG += c++11 link_pkgconfig
CONFIG -= app_bundle

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include($$PWD/../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../common/\
        -I /usr/include/mate-desktop-2.0/

PKGCONFIG += \
        glib-2.0\
        gio-2.0\
        gobject-2.0\
        dbus-glib-1\
        gmodule-2.0

LIBS += \
        -lmate-desktop-2

SOURCES += \
        $$PWD/main.cpp\
        $$PWD/plugin-info.cpp\
        $$PWD/plugin-manager.cpp\
#        $$PWD/plugin-manager-adaptor.cpp

HEADERS += \
        $$PWD/global.h\
        $$PWD/plugin-info.h\
        $$PWD/plugin-manager.h\
#        $$PWD/plugin-manager-adaptor.h

