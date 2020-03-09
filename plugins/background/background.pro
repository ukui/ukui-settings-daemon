TEMPLATE = lib
TARGET = background

QT -= gui
CONFIG += c++11 plugin
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common

SOURCES += \
        $$PWD/backgroundplugin.cpp

HEADERS += \
        $$PWD/backgroundplugin.h

#target.path = /usr/lib/ukui-settings-daemon/

#INSTALLS += target
#DESTDIR = $$OUT_PWD/../../library/
DESTDIR = /home/dingjing/q
