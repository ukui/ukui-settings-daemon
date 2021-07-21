#-------------------------------------------------
#
# Project created by QtCreator 2020-04-16T09:30:00
#
#-------------------------------------------------
QT += core widgets dbus x11extras gui

TEMPLATE = lib
TARGET = background

CONFIG += no_keywords c++11 create_prl plugin link_pkgconfig app_bundle debug

DEFINES += MODULE_NAME=\\\"backGround\\\"

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gio-2.0 \
        gtk+-3.0 \
        glib-2.0 \
        imlib2

SOURCES += \
        $$PWD/background-plugin.cpp \
        $$PWD/background-manager.cpp

HEADERS += \
        $$PWD/background-plugin.h \
        $$PWD/background-manager.h

background_lib.path = $${PLUGIN_INSTALL_DIRS}
background_lib.files += $$OUT_PWD/libbackground.so

INSTALLS += background_lib
