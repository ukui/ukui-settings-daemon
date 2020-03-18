#-------------------------------------------------
#
# Project created by QtCreator 2020-03-16T20:31:34
#
#-------------------------------------------------

QT       -= gui

TARGET = common
TEMPLATE = lib

DEFINES += COMMON_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

PKGCONFIG += glib-2.0

include($$PWD/../../common/common.pri)

PKGCONFIG +=     glib-2.0 \
    atk    \
    gio-2.0 \
    gdk-3.0 \


SOURCES += \
        ukui-osd-window.cpp \
    ukui-input-helper.c \
    ukui-keygrab.c

HEADERS += \
        ukui-osd-window.h \
        common_global.h \ 
    usd-input-helper.h \
    eggaccelerators.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
