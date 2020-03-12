#ifndef MANAGER_INTERFACE_H
#define MANAGER_INTERFACE_H

#include "plugin-manager.h"

#include <QMap>
#include <QList>
#include <QObject>
#include <QString>
#include <QtDBus>
#include <QVariant>
#include <QByteArray>
#include <QStringList>

namespace UkuiSettingsDaemon {
class PluginManagerDBus;
}

/*
 * Proxy class for interface org.ukui.SettingsDaemon
 */
class PluginManagerDBus: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.ukui.SettingsDaemon"; }

public:
    PluginManagerDBus(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~PluginManagerDBus();

public slots:
    inline QDBusPendingReply<bool> managerAwake()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerAwake"), argumentList);
    }

    inline QDBusPendingReply<bool> managerStart()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerStart"), argumentList);
    }

    inline QDBusPendingReply<> managerStop()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerStop"), argumentList);
    }

    inline QDBusPendingReply<QString> onPluginActivated()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("onPluginActivated"), argumentList);
    }

    inline QDBusPendingReply<QString> onPluginDeactivated()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("onPluginDeactivated"), argumentList);
    }

signals: // SIGNALS
};

#endif
