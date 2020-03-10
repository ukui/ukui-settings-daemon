TEMPLATE = lib
TARGET = a11y-settings

QT -= gui
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

## Default rules for deployment.
#unix {
#    target.path = /usr/lib
#}
#!isEmpty(target.path): INSTALLS += target
DESTDIR = /home/dingjing/q
