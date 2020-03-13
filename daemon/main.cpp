#include <stdio.h>
#include <libmate-desktop/mate-gsettings.h>

#include "clib-syslog.h"
#include "plugin-manager.h"
#include "manager-interface.h"

#include <QDebug>
#include <QDBusReply>
#include <QApplication>
#include <QDBusConnectionInterface>

static void print_help ();
static void parse_args (int argc, char *argv[]);
static void stop_daemon ();

static bool no_daemon       = true;
static bool replace         = false;

int main (int argc, char* argv[])
{
    PluginManager*          manager = NULL;

    syslog_init("ukui-settings-daemon", LOG_LOCAL6);

    CT_SYSLOG(LOG_DEBUG, "starting...");

    QApplication app(argc, argv);
    parse_args (argc, argv);

    if (replace) stop_daemon ();

    manager = PluginManager::getInstance();
    if (nullptr == manager) {
        CT_SYSLOG(LOG_ERR, "get plugin manager error");
        goto out;
    }

    if (!manager->managerStart()) {
        CT_SYSLOG(LOG_ERR, "manager start error!");
        goto out;
    }

    return app.exec();
out:

    if (manager != NULL) delete manager;

    CT_SYSLOG(LOG_DEBUG, "SettingsDaemon finished");

    return -1;
}

static void parse_args (int argc, char *argv[])
{
    if (argc == 1) return;

    for (int i = 1; i < argc; ++i) {
        if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--replace"))) {
            replace = true;
        } else if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--daemon"))) {
            no_daemon = false;
        } else {
            if (argc > 1) {
                print_help();
                CT_SYSLOG(LOG_DEBUG, " Unsupported command line arguments: '%s'", argv[i]);
                QApplication::exit(0);
            }
        }
    }
}

static void print_help()
{
    fprintf(stdout, "%s\n%s\n%s\n%s\n\n", \
                "Useage: ukui-setting-daemon <option> [...]", \
                "options:",\
                "    --replace   Replace the current daemon", \
                "    --daemon    Become a daemon(not support now)");
}


static void stop_daemon ()
{
    QString ukuiDaemonBusName = UKUI_SETTINGS_DAEMON_DBUS_NAME;
    QString ukuiDaemonBusPath = UKUI_SETTINGS_DAEMON_DBUS_PATH;

    bool isStarting = QDBusConnection::sessionBus().interface()->isServiceRegistered(ukuiDaemonBusName);
    if (isStarting) {
        // close current
        PluginManagerDBus pmd(ukuiDaemonBusName, ukuiDaemonBusPath, QDBusConnection::sessionBus());
        QDBusPendingReply<> reply = pmd.managerStop();
        reply.waitForFinished();
        if (reply.isValid()) {
            CT_SYSLOG(LOG_DEBUG, "stop current 'ukui-settings-daemon'");
        }
    }
}

