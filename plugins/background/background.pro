TEMPLATE = lib
TARGET = background

QT -= gui
CONFIG += no_keywords c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \

PKGCONFIG += \
        gio-2.0 \
        glib-2.0 \
        mate-desktop-2.0

SOURCES += \
        $$PWD/backgroundplugin.cpp \
        background-manager.cpp

HEADERS += \
        $$PWD/backgroundplugin.h \
        background-manager.h

DESTDIR = $$PWD/

background_lib.path = /usr/local/lib/ukui-settings-daemon/
background_lib.files = $$PWD/libbackground.so

INSTALLS += background_lib
