#-------------------------------------------------
#
# Project created by QtCreator 2020-11-12T15:37:31
#
#-------------------------------------------------

QT       +=xml core gui dbus network KWindowSystem KScreen

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = save-param
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"save-param\\\"



LIBS += -lX11 -lgsettings-qt


CONFIG += link_pkgconfig
CONFIG += C++11

PKGCONFIG += mate-desktop-2.0 \

PKGCONFIG += glib-2.0  gio-2.0 libxklavier x11 xrandr xtst atk gdk-3.0 gtk+-3.0 xi


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

target.source += $$TARGET
target.path = /usr/bin


INSTALLS +=  \
            target

SOURCES += \
    xrandr-config.cpp \
    xrandr-output.cpp   \
    kdswidget.cpp \
    main.cpp

HEADERS += \
    xrandr-config.h \
    xrandr-output.h \
    kdswidget.h



TRANSLATIONS +=  \
                res/zh_CN.ts \

include($$PWD/../../common/common.pri)
