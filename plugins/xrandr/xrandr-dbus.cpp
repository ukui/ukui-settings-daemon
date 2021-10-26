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

#include "xrandr-manager.h"

#define UKUI_DAEMON_NAME    "ukui-settings-daemon"
XrandrManager *xrandrManager = nullptr;
xrandrDbus::xrandrDbus(QObject* parent) : QObject(parent)
{
    mXsettings =new QGSettings("org.ukui.SettingsDaemon.plugins.xsettings");
    mScale = mXsettings->get("scaling-factor").toDouble();
    xrandrManager = static_cast<XrandrManager*>(parent);

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
    USD_LOG(LOG_DEBUG,"get screen mode appName:%s:%d", appName.toLatin1().data());
    return xrandrManager->discernScreenMode();
}

int xrandrDbus::setScreensParam(QString screensParam, QString appName)
{
    USD_LOG(LOG_DEBUG,".appName:%s",screensParam.toLatin1().data(),appName);
    Q_EMIT setScreensParamSignal(screensParam);
    return 1;
}

QString xrandrDbus::getScreensParam(QString appName)
{
    USD_LOG(LOG_DEBUG,"dbus from %s",appName);
    return xrandrManager->getScreesParam();
}

void xrandrDbus::sendModeChangeSignal(int screensMode)
{
    USD_LOG(LOG_DEBUG,"send mode:%d",screensMode);
    Q_EMIT screenModeChanged(screensMode);
}

void xrandrDbus::sendScreensParamChangeSignal(QString screensParam)
{
//    USD_LOG(LOG_DEBUG,"send param:%s",screensParam.toLatin1().data());
     Q_EMIT screensParamChanged(screensParam);
}


