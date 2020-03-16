QT -= gui

TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle
PKGCONFIG += glib-2.0 \
             gio-2.0 \

INCLUDEPATH += \
    -I /usr/include/pulse \
    -I $$PWD/../../common

LIBS += \
    -lpulse \
    -L /usr/lib/x86_64-linux-gnu/

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

SOURCES += \
    soundmanager.cpp \
    soundplugin.cpp

DISTFILES += \
    sound.ukui-settings-plugin.in

HEADERS += \
    soundmanager.h \
    soundplugin.h

DESTDIR = $$PWD/../../library/
