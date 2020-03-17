QT -= gui

TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
    -I $$PWD/../../common/

PKGCONFIG += \
    glib-2.0 \
    gio-2.0

SOURCES += \
    mprismanager.cpp \
    mprisplugin.cpp

HEADERS += \
    mprismanager.h \
    mprisplugin.h

DESTDIR = \
    $$OUT_PWD/../../library/
