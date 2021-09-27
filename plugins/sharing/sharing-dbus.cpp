#include "sharing-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>

sharingDbus::sharingDbus(QObject *parent) : QObject(parent)
{

}

sharingDbus::~sharingDbus()
{

}

void sharingDbus::EnableService(QString serviceName)
{
    qDebug()<<__func__<<serviceName;
    Q_EMIT serviceChange("enable", serviceName);
}

void sharingDbus::DisableService(QString serviceName)
{
    qDebug()<<__func__<<serviceName;
    Q_EMIT serviceChange("disable", serviceName);
}
