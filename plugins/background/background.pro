TEMPLATE = lib
TARGET = background

QT -= gui
CONFIG += no_keywords c++11 create_prl plugin link_pkgconfig debug
CONFIG -= app_bundle

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

DESTDIR = $$PWD

background_lib.path = /usr/local/lib/ukui-settings-daemon/
background_lib.files += \
    $$PWD/libbackground.prl \
    $$PWD/libbackground.so \

INSTALLS += background_lib
