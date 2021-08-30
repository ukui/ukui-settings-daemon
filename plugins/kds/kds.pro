#-------------------------------------------------
#
# Project created by QtCreator 2020-11-12T15:37:31
#
#-------------------------------------------------

QT       += core gui dbus network KWindowSystem KScreen

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = kydisplayswitch
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


include (qtsingleapplication/qtsingleapplication.pri)

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
gsetting.files += ../conf/org.ukui.kds.gschema.xml
gsetting.path = /usr/share/glib-2.0/schemas
desktop.files += ../kylin-display-switch-daemon.desktop
desktop.files += ../kylin-display-switch-mkt.desktop
desktop.path = /etc/xdg/autostart/
service.files += ../conf/kylin-display-switch.service
service.path = /usr/lib/systemd/system/

INSTALLS +=  \
            target \
            gsetting \
            desktop \
            service \

SOURCES += \
    kdswidget.cpp \
        main.cpp \
        widget.cpp \
    expendbutton.cpp \
    xeventmonitor.cpp

HEADERS += \
    kdswidget.h \
        widget.h \
    expendbutton.h \
    xeventmonitor.h

FORMS += \
        kdswidget.ui \
        widget.ui

RESOURCES += \
    res/img.qrc

TRANSLATIONS +=  \
                res/zh_CN.ts \
