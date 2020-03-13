QT += core gui dbus
CONFIG += c++11 no_keywords link_pkgconfig
CONFIG -= app_bundle

include($$PWD/../common/common.pri)

INCLUDEPATH += -I $$PWD/

PKGCONFIG += \
        glib-2.0\
        gio-2.0


SOURCES += \
    $$PWD/clib-syslog.c\
    $$PWD/QGSettings/qconftype.cpp\
    $$PWD/QGSettings/qgsettings.cpp

HEADERS += \
    $$PWD/clib-syslog.h \
    $$PWD/plugin-interface.h\
    $$PWD/QGSettings/qconftype.h\
    $$PWD/QGSettings/qgsettings.h
