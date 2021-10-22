#include "xrandr-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QDebug>
#include <QAction>
#include <KF5/KGlobalAccel/KGlobalAccel>

#include <glib.h>
#include <gio/gio.h>

#include "clib-syslog.h"
#include "usd_base_class.h"


#define UKUI_DAEMON_NAME    "ukui-settings-daemon"

xrandrDbus::xrandrDbus(QObject* parent) : QObject(parent)
{
    mXsettings =new QGSettings("org.ukui.SettingsDaemon.plugins.xsettings");
    mScale = mXsettings->get("scaling-factor").toDouble();
}
xrandrDbus::~xrandrDbus()
{
    delete mXsettings;
}

int xrandrDbus::setScreenMode(QString modeName,QString appName){
    USD_LOG(LOG_DEBUG,"change screen :%s, appName:%s",modeName.toLatin1().data(), appName.toLatin1().data());
    Q_EMIT setScreenModeSignal(modeName);
    return true;
}

int xrandrDbus::getScreenMode(QString appName){
    USD_LOG(LOG_DEBUG,"get screen mode appName:%s:%d", appName.toLatin1().data(),mScreenMode);
    return mScreenMode;
}

int xrandrDbus::setScreensParam(QString screensParam, QString appName)
{
    USD_LOG(LOG_DEBUG,".appName:%s",screensParam.toLatin1().data(),appName);
    Q_EMIT setScreensParamSignal(screensParam);
    return 1;
}

QString xrandrDbus::getScreensParam(QString appName)
{
    USD_LOG(LOG_DEBUG,".");
    return QString::fromUtf8(__FUNCTION__);
}





