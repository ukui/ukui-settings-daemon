QT -= gui
TEMPLATE = lib
TARGET = dummy

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += glib-2.0 \
             gio-2.0 \

INCLUDEPATH += \
        -I $$PWD/../../common

SOURCES += \
    dummymanager.cpp \
    dummyplugin.cpp

HEADERS += \
    dummymanager.h \
    dummyplugin.h

DESTDIR = $$PWD/../../library/
