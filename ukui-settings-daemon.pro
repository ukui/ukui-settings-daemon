TEMPLATE = app
QT += core
CONFIG += console c++11
CONFIG -= app_bundle
#CONFIG -= qt

INCLUDEPATH += \
        -I ukui-settings-daemon/ \
        -I /usr/include/glib-2.0/ \
        -I /usr/include/dbus-1.0


SOURCES += \
        ukui-settings-daemon/main.cpp \
        ukui-settings-daemon/ukuisettingsmanager.cpp \
        ukui-settings-daemon/ukuisettingsmodule.cpp \
        ukui-settings-daemon/ukuisettingsplugin.cpp \
        ukui-settings-daemon/ukuisettingsplugininfo.cpp \
        ukui-settings-daemon/ukuisettingsprofile.cpp

HEADERS += \
    ukui-settings-daemon/ukuisettingsmanager.h \
    ukui-settings-daemon/ukuisettingsmodule.h \
    ukui-settings-daemon/ukuisettingsplugin.h \
    ukui-settings-daemon/ukuisettingsplugininfo.h \
    ukui-settings-daemon/ukuisettingsprofile.h
