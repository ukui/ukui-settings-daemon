QT -= gui
QT += core dbus
CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TEMPLATE = app
TARGET = authoritydbus
DESTDIR = .


target.source += $$TARGET
target.path = /usr/bin
service.files += conf/com.settings.daemon.qt.systemdbus.service
service.path = /usr/share/dbus-1/system-services/
conffile.files += conf/com.settings.daemon.qt.systemdbus.conf
conffile.path = /etc/dbus-1/system.d/


INSTALLS +=  \
            target \
            service \
            conffile


SOURCES += \
        authorityservice.cpp \
        main.cpp

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

HEADERS += \
    authorityservice.h

DISTFILES += \
    conf/com.settings.daemon.qt.systemdbus.conf \
    conf/com.settings.daemon.qt.systemdbus.service
