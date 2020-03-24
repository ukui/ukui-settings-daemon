TEMPLATE = lib
TARGET = typing-break

QT -= gui
CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

PKGCONFIG += glib-2.0 \
             gio-2.0

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common

SOURCES += \
    typingbreakmanager.cpp \
    typingbreakplugin.cpp

HEADERS += \
    typingbreakmanager.h \
    typingbreakplugin.h

DESTDIR = $$PWD/

typing_break_lib.path = /usr/local/lib/ukui-settings-daemon/
typing_break_lib.files = $$PWD/libtyping-break.so

INSTALLS += typing_break_lib
