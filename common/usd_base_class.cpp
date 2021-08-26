#include <QX11Info>
#include <QGuiApplication>
#include "clib-syslog.h"
#include "usd_base_class.h"

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

bool UsdBaseClass::isIntel()
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
