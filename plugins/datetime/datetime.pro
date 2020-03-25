QT -= gui

CONFIG += c++11 console link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += glib-2.0 \
             gio-2.0 \
             dbus-glib-1 \
             polkit-agent-1 \

INCLUDEPATH += \
    $$PWD/../../common \

include($$PWD/../../common/common.pri)

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    datetime-mechanism-main.cpp \
    datetime-mechanism.cpp \
    system-timezone.cpp

HEADERS += \
    datetime-mechanism.h \
    system-timezone.h

