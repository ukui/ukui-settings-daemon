TEMPLATE = lib
TARGET = clipboard

QT += gui
CONFIG += no_keywords c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \

PKGCONFIG += \
        gdk-3.0

SOURCES += \
    $$PWD/mediakey-plugin.cpp \
    mediakey-manager.cpp

HEADERS += \
    $$PWD/mediakey-plugin.h \
    mediakey-manager.h


DESTDIR = $$OUT_PWD/../../library/
