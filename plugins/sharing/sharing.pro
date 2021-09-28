#-------------------------------------------------
#
# Project created by QtCreator 2021-08-30T19:50:48
#
#-------------------------------------------------

QT  += gui dbus

TARGET = sharing
TEMPLATE = lib

DEFINES += SHARING_LIBRARY

CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"sharing\\\"

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/../../common/common.pri)


PKGCONFIG +=\
    gsettings-qt

SOURCES += \
    sharing-plugin.cpp  \
    sharing-manager.cpp \
    sharing-dbus.cpp    \
    sharing-adaptor.cpp

HEADERS += \
    sharing-plugin.h    \
    sharing-manager.h   \
    sharing-dbus.h      \
    sharing-adaptor.h

sharing_lib.path = $${PLUGIN_INSTALL_DIRS}
sharing_lib.files += $$OUT_PWD/libsharing.so \

INSTALLS += sharing_lib

