#-------------------------------------------------
#
# Project created by QtCreator 2020-04-16T09:30:00
#
#-------------------------------------------------
QT -= gui
TEMPLATE = lib
TARGET = sound

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle
DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"sound\\\"

include($$PWD/../../common/common.pri)

PKGCONFIG += glib-2.0 \
             gio-2.0 \

INCLUDEPATH += \
    -I /usr/include/pulse 

LIBS += \
    -lpulse 


SOURCES += \
    sound-manager.cpp \
    sound-plugin.cpp

DISTFILES += \
    sound.ukui-settings-plugin.in

HEADERS += \
    sound-manager.h \
    sound-plugin.h

sound_lib.path = $${PLUGIN_INSTALL_DIRS}
sound_lib.files = $$OUT_PWD/libsound.so

INSTALLS += sound_lib
