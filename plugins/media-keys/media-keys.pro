TEMPLATE = lib
TARGET = media-keys

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

DESTDIR = $$PWD/

media_keys_lib.path = /usr/local/lib/ukui-settings-daemon/
media_keys_lib.files = $$PWD/libclipboard.so

INSTALLS += media_keys_lib
