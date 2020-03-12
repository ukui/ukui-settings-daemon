#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libmate-desktop/mate-gsettings.h>

#include "plugin-manager.h"
#include "clib-syslog.h"

#include <QDebug>
#include <QApplication>

static void print_help();
static void parse_args (int argc, char *argv[]);

static bool no_daemon       = true;
static bool replace         = false;
static bool debug           = false;

int main (int argc, char* argv[])
{
    PluginManager*          manager = NULL;

    syslog_init("ukui-settings-daemon", LOG_LOCAL6);

    CT_SYSLOG(LOG_DEBUG, "starting...");

    // bindtextdomain (GETTEXT_PACKAGE, UKUI_SETTINGS_LOCALEDIR);
    // bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    // textdomain (GETTEXT_PACKAGE);
    // setlocale (LC_ALL, "");

    QApplication app(argc, argv);

    parse_args (argc, argv);

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

    if (manager != NULL) g_object_unref (manager);

    CT_SYSLOG(LOG_DEBUG, "SettingsDaemon finished");

    return 0;
}

static void parse_args (int argc, char *argv[])
{
    if (argc == 1) return;

    for (int i = 1; i < argc; ++i) {
        if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--replace"))) {
            replace = true;
        } else if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--debug"))) {
            debug = true;
        } else if (0 == QString::compare(QString(argv[i]).trimmed(), QString("--daemon"))) {
            no_daemon = false;
        } else {
            if (argc > 1) {
                print_help();
                CT_SYSLOG(LOG_DEBUG, " Unsupported command line arguments: '%s'", argv[i]);
                exit (-1);
            }
        }
    }
}

// FIXME://
static void print_help()
{
    fprintf(stdout, "%s\n%s\n%s\n%s\n%s\n\n", \
                "Useage: ukui-setting-daemon <option> [...]", \
                "options:",\
                "    --replace   Replace the current daemon", \
                "    --debug     Enable debugging code", \
                "    --daemon    Become a daemon");
}

