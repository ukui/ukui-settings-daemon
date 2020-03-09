TEMPLATE = lib
TARGET = baseplugin

QT -= gui
CONFIG += c++11 plugin
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        $$PWD/clib_syslog.c\
        $$PWD/ukuisettingsplugin.cpp


HEADERS += \
        $$PWD/clib_syslog.h\
        $$PWD/ukuisettingsplugin.h
