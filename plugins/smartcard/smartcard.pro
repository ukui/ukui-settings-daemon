TEMPLATE = lib
TARGET = smartcard
DEFINES += SMARTCARD_LIBRARY

QT -= gui
CONFIG += c++11 no_keywords plugin link_pkgcinfig

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)
INCLUDEPATH += \

PKGCONFIG += \
        gdk-3.0 \
        glib-2.0 \
	nspr   nss

SOURCES += \
    smartcard-manager.cpp \
    smartcard-plugin.cpp \
    usd-smartcard.cpp

HEADERS += \
    smartcard-manager.h \
    smartcard_global.h \
    smartcard-plugin.h \
    usd-smartcard.h

DESTDIR = $$PWD/

smartcard_lib.path = /usr/local/lib/ukui-settings-daemon/
smartcard_lib.files = $$PWD/libsmartcard.so

INSTALLS += smartcard_lib
