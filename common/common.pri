#TEMPLATE = lib
#TARGET = baseplugin

#QT -= gui
#CONFIG += c++11 plugin
#CONFIG -= app_bundle
#DEFINES += SHAREDLIB_LIBRARY

#DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    $$PWD/clib-syslog.c


HEADERS += \
    $$PWD/global.h\
    $$PWD/clib-syslog.h \
    $$PWD/plugin-interface.h

#DESTDIR = $$OUT_PWD/../library/
