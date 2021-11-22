/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
//#include <libmate-desktop/mate-gsettings.h>

#include "clib-syslog.h"
#include "plugin-manager.h"
#include "manager-interface.h"

#include <QDebug>
#include <QObject>
#include <QDBusReply>
#include <QApplication>
#include <QDBusConnectionInterface>

#include <sys/types.h>
#include <signal.h>

static void print_help ();
static void parse_args (int argc, char *argv[]);
static void stop_daemon ();

static bool no_daemon       = true;
static bool replace         = false;
PluginManager*  manager = nullptr;
void handler(int no)
{
    USD_LOG(LOG_DEBUG,"catch SIGTERM signal, with exitcode %d",no);
//    manager->managerStop();
    QApplication::exit(0);
}

int main (int argc, char* argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif


    QApplication app(argc, argv);

    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);


    signal(SIGTERM, &handler);
    QApplication::setQuitOnLastWindowClosed(false);

    QTranslator translator;
    translator.load("/usr/share/ukui-settings-daemon/daemon/res/i18n/zh_CN.qm");
    app.installTranslator(&translator);
    parse_args (argc, argv);

    if (replace) stop_daemon ();

    manager = PluginManager::getInstance();
    if (!manager) {
        return 0;
    }

    bool res = manager->managerStart();
    if (!res) {
         USD_LOG(LOG_INFO, "manager start error!");
        return 0;
    }

    USD_LOG(LOG_INFO, "ukui-settings-daemon started!");
    return app.exec();
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
                USD_LOG(LOG_DEBUG, " Unsupported command line arguments: '%s'", argv[i]);
                exit(0);
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
            //USD_LOG(LOG_DEBUG, "stop current 'ukui-settings-daemon'");
        }
    }
}
