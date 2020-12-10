#-------------------------------------------------
#
# Project created by QtCreator 2020-08-05T19:30:00
#
#-------------------------------------------------
QT += gui core widgets x11extras
TARGET = a11y-keyboard
TEMPLATE = lib
DEFINES += A11YKEYBOARD_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin
CONFIG += app_bunale

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

PKGCONFIG += \
        gtk+-3.0  \
        glib-2.0  \
        libnotify \
        gsettings-qt \
        xi 

SOURCES += \
    a11y-keyboard-manager.cpp \
    a11y-keyboard-plugin.cpp \
    a11y-preferences-dialog.cpp

HEADERS += \
    a11y-keyboard-manager.h \
    a11y-keyboard-plugin.h \
    a11y-preferences-dialog.h

a11y_keyboard_lib.path = $${PLUGIN_INSTALL_DIRS}
a11y_keyboard_lib.files = $$OUT_PWD/liba11y-keyboard.so

INSTALLS += a11y_keyboard_lib

FORMS += \
    a11y-preferences-dialog.ui
