#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libmate-desktop/mate-gsettings.h>

#include "plugin-manager.h"
#include "clib-syslog.h"

#include <QApplication>

#define USD_DBUS_NAME                           "org.ukui.SettingsDaemon"
#define DEBUG_KEY                               "mate-settings-daemon"
#define DEBUG_SCHEMA                            "org.mate.debug"

#define UKUI_SESSION_DBUS_NAME                  "org.gnome.SessionManager"
#define UKUI_SESSION_DBUS_OBJECT                "/org/gnome/SessionManager"
#define UKUI_SESSION_DBUS_INTERFACE             "org.gnome.SessionManager"
#define UKUI_SESSION_PRIVATE_DBUS_INTERFACE     "org.gnome.SessionManager.ClientPrivate"

static DBusGConnection* get_session_bus (void);
static gboolean bus_register (DBusGConnection *bus);

static void on_session_query_end (DBusGProxy *proxy, guint flags, PluginManager *manager);
static void on_session_end (DBusGProxy *proxy, guint flags, PluginManager *manager);
static DBusGProxy* get_bus_proxy (DBusGConnection *connection);
static gboolean acquire_name_on_proxy (DBusGProxy *bus_proxy);
static void set_session_over_handler (DBusGConnection *bus, PluginManager *manager);
static DBusHandlerResult bus_message_handler (DBusConnection *connection, DBusMessage* message, void* user_data);
static gboolean timed_exit_cb (void);

static void parse_args (int *argc, char ***argv);
static void debug_changed (GSettings *settings, gchar *key, gpointer user_data);

static gboolean   no_daemon    = TRUE;
static gboolean   replace      = FALSE;
static gboolean   debug        = FALSE;
static gboolean   do_timed_exit = FALSE;

static GOptionEntry entries[] = {
        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, ("Enable debugging code"), NULL },
        { "replace", 0, 0, G_OPTION_ARG_NONE, &replace, ("Replace the current daemon"), NULL },
        { "no-daemon", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &no_daemon, ("Don't become a daemon"), NULL },
        { "timed-exit", 0, 0, G_OPTION_ARG_NONE, &do_timed_exit, ("Exit after a time (for debugging)"), NULL },
        { NULL },
};

int main (int argc, char* argv[])
{
    PluginManager*          manager = NULL;
    DBusGConnection*        bus = NULL;
    gboolean                res;
    GError*                 error = NULL;
    GSettings*              debug_settings = NULL;

    syslog_init("ukui-settings-daemon", LOG_LOCAL6);

    CT_SYSLOG(LOG_DEBUG, "starting...");

    // bindtextdomain (GETTEXT_PACKAGE, UKUI_SETTINGS_LOCALEDIR);
    // bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    // textdomain (GETTEXT_PACKAGE);
    // setlocale (LC_ALL, "");

    // FIXME:// QT parse command line
    parse_args (&argc, &argv);

    QApplication app(argc, argv);

    /* Allows to enable/disable debug from GSettings only if it is not set from argument */
    if (mate_gsettings_schema_exists (DEBUG_SCHEMA)) {
        debug_settings = g_settings_new (DEBUG_SCHEMA);
        debug = g_settings_get_boolean (debug_settings, DEBUG_KEY);
        g_signal_connect (debug_settings, "changed::" DEBUG_KEY, G_CALLBACK (debug_changed), NULL);
        if (debug) {
            g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
        }
    }

    CT_SYSLOG(LOG_DEBUG, "dbus session...");
    bus = get_session_bus ();
    if (bus == NULL) {
        CT_SYSLOG(LOG_ERR, "Could not get a connection to the bus!");
        goto out;
    }

    CT_SYSLOG(LOG_DEBUG, "dbus register...");
    if (!bus_register (bus)) {
        CT_SYSLOG(LOG_ERR, "dbus register error!");
        goto out;
    }

    manager = PluginManager::getInstance();
    if (manager == NULL) {
        CT_SYSLOG(LOG_ERR, "Unable to register object");
        goto out;
    }

    CT_SYSLOG(LOG_DEBUG, "set session handler...");
    set_session_over_handler (bus, manager);

    /* If we aren't started by dbus then load the plugins automatically.  Otherwise, wait for an Awake etc. */
    if (g_getenv ("DBUS_STARTER_BUS_TYPE") == NULL) {
        error = NULL;
        res = manager->managerStart();
        if (! res) {
            CT_SYSLOG(LOG_ERR, "Unable to start: %s", error->message);
            g_error_free (error);
            goto out;
        }
    }
    // FIXME:// maybe error
    if (do_timed_exit) {
        g_timeout_add_seconds (30, (GSourceFunc) timed_exit_cb, NULL);
    }

    return app.exec();
out:

    if (bus != NULL) dbus_g_connection_unref (bus);
    if (manager != NULL) g_object_unref (manager);
    if (debug_settings != NULL) g_object_unref (debug_settings);

    CT_SYSLOG(LOG_DEBUG, "SettingsDaemon finished");

    return 0;
}

static void parse_args (int *argc, char ***argv)
{
    GError *error;
    GOptionContext *context;

    context = g_option_context_new (NULL);

    g_option_context_add_main_entries (context, entries, NULL);
    //g_option_context_add_group (context, gtk_get_option_group (FALSE));

    error = NULL;
    if (!g_option_context_parse (context, argc, argv, &error)) {
        if (error != NULL) {
            CT_SYSLOG(LOG_ERR, "%s", error->message);
            g_error_free (error);
        } else {
            CT_SYSLOG(LOG_ERR, "Unable to initialize GTK+");
        }
        exit (EXIT_FAILURE);
    }

    g_option_context_free (context);

    // FIXME:// debug log not use
//    if (debug) g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
}

static void debug_changed (GSettings *settings, gchar *key, gpointer user_data)
{
    debug = g_settings_get_boolean (settings, DEBUG_KEY);
    if (debug) {
        CT_SYSLOG(LOG_ERR, "Enable DEBUG");
        g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
    } else {
        CT_SYSLOG(LOG_ERR, "Disable DEBUG");
        g_unsetenv ("G_MESSAGES_DEBUG");
    }
}

static DBusGConnection* get_session_bus (void)
{
    GError          *error;
    DBusGConnection *bus;
    DBusConnection  *connection;

    error = NULL;
    bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (bus == NULL) {
        CT_SYSLOG(LOG_ERR, "Couldn't connect to session bus: %s", error->message);
        g_error_free (error);
        goto out;
    }

    connection = dbus_g_connection_get_connection (bus);
    dbus_connection_add_filter (connection, (DBusHandleMessageFunction) bus_message_handler, NULL, NULL);
    dbus_connection_set_exit_on_disconnect (connection, FALSE);

out:
    return bus;
}

static void on_session_over (DBusGProxy *proxy, PluginManager *manager)
{
    /* not used, see on_session_end instead */
}

static void set_session_over_handler (DBusGConnection* bus, PluginManager* manager)
{
    DBusGProxy *session_proxy;
    DBusGProxy *private_proxy;
    gchar *client_id = NULL;
    const char *startup_id;
    GError *error = NULL;
    gboolean res;

    if (NULL != bus) QApplication::exit();

    session_proxy = dbus_g_proxy_new_for_name (bus, UKUI_SESSION_DBUS_NAME, UKUI_SESSION_DBUS_OBJECT, UKUI_SESSION_DBUS_INTERFACE);
    dbus_g_object_register_marshaller(g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(session_proxy, "SessionOver", G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(session_proxy, "SessionOver", G_CALLBACK (on_session_over), manager, NULL);

    /* Register with ukui-session */
    startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
    if (startup_id != NULL && *startup_id != '\0') {
        res = dbus_g_proxy_call (session_proxy, "RegisterClient", &error, G_TYPE_STRING, "ukui-settings-daemon",
                     G_TYPE_STRING, startup_id, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &client_id, G_TYPE_INVALID);
        if (!res) {
            CT_SYSLOG(LOG_ERR, "failed to register client '%s': %s", startup_id, error->message);
            g_error_free (error);
        } else {
            /* get org.gnome.SessionManager.ClientPrivate interface */
            private_proxy = dbus_g_proxy_new_for_name_owner (bus, UKUI_SESSION_DBUS_NAME,
                         client_id, UKUI_SESSION_PRIVATE_DBUS_INTERFACE, &error);
            if (private_proxy == NULL) {
                CT_SYSLOG(LOG_ERR, "DBUS error: %s", error->message);
                g_error_free (error);
            } else {
                /* get QueryEndSession */
                dbus_g_proxy_add_signal (private_proxy, "QueryEndSession", G_TYPE_UINT, G_TYPE_INVALID);
                dbus_g_proxy_connect_signal (private_proxy, "QueryEndSession", G_CALLBACK (on_session_query_end), manager, NULL);
                /* get EndSession */
                dbus_g_proxy_add_signal (private_proxy, "EndSession", G_TYPE_UINT, G_TYPE_INVALID);
                dbus_g_proxy_connect_signal (private_proxy, "EndSession", G_CALLBACK (on_session_end), manager, NULL);
            }
            g_free (client_id);
        }
    }

    // FIXME://
    // watch_for_term_signal (manager);
}

static void on_session_query_end (DBusGProxy *proxy, guint flags, PluginManager *manager)
{
    GError *error = NULL;
    gboolean ret = FALSE;

    /* send response */
    ret = dbus_g_proxy_call (proxy, "EndSessionResponse", &error, G_TYPE_BOOLEAN, TRUE /* ok */, G_TYPE_STRING, NULL /* reason */, G_TYPE_INVALID, G_TYPE_INVALID);
    if (!ret) {
        CT_SYSLOG(LOG_ERR, "failed to send session response: %s", error->message);
        g_error_free (error);
    }
}

static void on_session_end (DBusGProxy *proxy, guint flags, PluginManager *manager)
{
    GError *error = NULL;
    gboolean ret = FALSE;

    /* send response */
    ret = dbus_g_proxy_call (proxy, "EndSessionResponse", &error, G_TYPE_BOOLEAN, TRUE /* ok */, G_TYPE_STRING, NULL /* reason */, G_TYPE_INVALID, G_TYPE_INVALID);
    if (!ret) {
        CT_SYSLOG(LOG_ERR, "failed to send session response: %s", error->message);
        g_error_free (error);
    }

    // FIXME://
    manager->managerStop();
    QApplication::exit();
}

static gboolean bus_register (DBusGConnection *bus)
{
    DBusGProxy      *bus_proxy;
    gboolean         ret;

    ret = FALSE;

    bus_proxy = get_bus_proxy (bus);

    if (bus_proxy == NULL) {
        CT_SYSLOG(LOG_ERR, "Could not construct bus_proxy object");
        goto out;
    }

    ret = acquire_name_on_proxy (bus_proxy);
    g_object_unref (bus_proxy);

    if (!ret) {
        CT_SYSLOG(LOG_ERR, "Could not acquire name");
        goto out;
    }

    CT_SYSLOG(LOG_DEBUG, "Successfully connected to D-Bus");

out:
    return ret;
}

static DBusGProxy* get_bus_proxy (DBusGConnection *connection)
{
    DBusGProxy *bus_proxy;

    bus_proxy = dbus_g_proxy_new_for_name (connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    return bus_proxy;
}

static gboolean acquire_name_on_proxy (DBusGProxy *bus_proxy)
{
    GError     *error;
    guint       result;
    gboolean    res;
    gboolean    ret;
    guint32     flags;

    ret = FALSE;

    flags = DBUS_NAME_FLAG_DO_NOT_QUEUE|DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
    if (replace) flags |= DBUS_NAME_FLAG_REPLACE_EXISTING;

    error = NULL;
    res = dbus_g_proxy_call (bus_proxy, "RequestName", &error, G_TYPE_STRING, USD_DBUS_NAME, G_TYPE_UINT, flags, G_TYPE_INVALID, G_TYPE_UINT, &result, G_TYPE_INVALID);
    if (! res) {
        if (error != NULL) {
            CT_SYSLOG(LOG_ERR, "Failed to acquire %s: %s", USD_DBUS_NAME, error->message);
            g_error_free (error);
        } else {
            CT_SYSLOG(LOG_ERR, "Failed to acquire %s", USD_DBUS_NAME);
        }
        goto out;
    }

    if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        if (error != NULL) {
            CT_SYSLOG(LOG_ERR, "Failed to acquire %s: %s", USD_DBUS_NAME, error->message);
            g_error_free (error);
        } else {
            CT_SYSLOG(LOG_ERR, "Failed to acquire %s", USD_DBUS_NAME);
        }
        goto out;
    }

    ret = TRUE;

out:
    return ret;
}

static DBusHandlerResult bus_message_handler (DBusConnection *connection, DBusMessage* message, void* user_data)
{
    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        QApplication::exit();
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else if (dbus_message_is_signal (message, DBUS_INTERFACE_DBUS, "NameLost")) {
        CT_SYSLOG(LOG_ERR, "D-Bus name lost, quitting");
        QApplication::exit();
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean timed_exit_cb (void)
{
    QApplication::exit();
    return FALSE;
}
