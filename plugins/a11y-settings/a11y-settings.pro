TEMPLATE = lib
TARGET = a11y-settings

QT -= gui
QT += core
CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += glib-2.0 \
             gio-2.0 \

INCLUDEPATH += \
    -I $$PWD/../../common

LIBS +=

SOURCES += \
    $$PWD/a11ysettingsmanager.cpp \
    $$PWD/a11ysettingsplugin.cpp

HEADERS += \
    $$PWD/a11ysettingsmanager.h \
    $$PWD/a11ysettingsplugin.h

DESTDIR = $$PWD/

a11_settings_lib.path = /usr/local/lib/ukui-settings-daemon/
a11_settings_lib.files = $$PWD/liba11y-settings.so

INSTALLS += a11_settings_lib
