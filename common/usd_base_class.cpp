#include <QX11Info>
#include <QGuiApplication>
#include "clib-syslog.h"
#include "usd_base_class.h"


#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

#define DBUS_SERVICE "org.freedesktop.UPower"
#define DBUS_OBJECT "/org/freedesktop/UPower"
#define DBUS_INTERFACE "org.freedesktop.DBus.Properties"

UsdBaseClass::UsdBaseClass()
{

}

UsdBaseClass::~UsdBaseClass()
{

}

bool UsdBaseClass::isMasterSP1()
{
#ifdef USD_MASTER
    return true;
#endif

    return false;
}

bool UsdBaseClass::isTablet()
{
#ifdef USD_TABLET
    return true;
#endif
    return false;
}

bool UsdBaseClass::is9X0()
{
#ifdef USD_9X0
     return true;
#endif
    return false;
}

bool UsdBaseClass::isWayland()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))){
        USD_LOG(LOG_DEBUG,"is wayland app");
        return true;
    }

    USD_LOG(LOG_DEBUG,"is xcb app");
    return false;
}

bool UsdBaseClass::isXcb()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("xcb"))){
        USD_LOG(LOG_DEBUG,"is xcb app");
        return true;
    }
    return false;
}

bool UsdBaseClass::isNotebook()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE,DBUS_OBJECT,DBUS_INTERFACE,"Get");
    msg<<DBUS_SERVICE<<"LidlsPresent";
    QDBusMessage res = QDBusConnection::systemBus().call(msg);
    if(res.type()==QDBusMessage::ReplyMessage)
    {
        QVariant v = res.arguments().at(0);
        QDBusVariant dv = qvariant_cast<QDBusVariant>(v);
        QVariant result = dv.variant();
        qDebug()<<"LidlsPresent: "<<  result.toBool();
        return result.toBool();
    }
}
