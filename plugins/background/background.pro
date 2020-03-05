QT -= gui

CONFIG += c++11
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += \
        -I ../../daemon\
        -I /usr/include/atk-1.0/\
        -I /usr/include/gtk-3.0/\
        -I /usr/include/pango-1.0/\
        -I /usr/include/cairo/\
        -I /usr/include/gdk-pixbuf-2.0/\
        -I /usr/include/glib-2.0/\
        -I /usr/lib/glib-2.0/include\
        -I /usr/lib/x86_64-linux-gnu/glib-2.0/include/\
        -I /usr/include/dbus-1.0 \
        -I /usr/lib/dbus-1.0/include/\
        -I /usr/lib/x86_64-linux-gnu/dbus-1.0/include/\
        -I /usr/include/mate-desktop-2.0/

SOURCES += \
        ../../daemon/clib_syslog.c\
        backgroundmanager.cpp \
        backgroundplugin.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
        ../../daemon/clib_syslog.h\
        backgroundmanager.h \
        backgroundplugin.h
