QT += gui widgets svg x11extras
TEMPLATE = lib
CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += \
    glib-2.0 \
    gio-2.0 \
    gtk+-3.0 \
    gsettings-qt \
    libmatemixer

INCLUDEPATH += \
    $$PWD/../common

LIBS += \
    -lX11 -lXi \
    $$PWD/../common/libcommon.so

DEFINES += QT_DEPRECATED_WARNINGS HAVE_X11_EXTENSIONS_XKB_H

SOURCES += \
        devicewindow.cpp \
        eggaccelerators.c \
        main.cpp \
        mediakeysmanager.cpp \
        usd-input-helper.c \
        usd-keygrab.cpp \
        volumewindow.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    acme.h \
    devicewindow.h \
    eggaccelerators.h \
    mediakeysmanager.h \
    usd-input-helper.h \
    usd-keygrab.h \
    volumewindow.h

FORMS += \
    devicewindow.ui \
    volumewindow.ui

