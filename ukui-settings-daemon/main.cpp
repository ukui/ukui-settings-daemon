#include <iostream>
#include <syslog.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libmate-desktop/mate-gsettings.h>
#include <QtWidgets/QApplication>

#include "ukuisettingsmanager.h"


#define USD_DBUS_NAME         "org.ukui.SettingsDaemon"
#define DEBUG_KEY             "mate-settings-daemon"
#define DEBUG_SCHEMA          "org.mate.debug"

#define UKUI_SESSION_DBUS_NAME      "org.gnome.SessionManager"
#define UKUI_SESSION_DBUS_OBJECT    "/org/gnome/SessionManager"
#define UKUI_SESSION_DBUS_INTERFACE "org.gnome.SessionManager"
#define UKUI_SESSION_PRIVATE_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

static DBusGConnection* get_session_bus (void);
static gboolean bus_register (DBusGConnection *bus);

static void on_session_query_end (DBusGProxy *proxy, guint flags, UkuiSettingsManager *manager);
static void on_session_end (DBusGProxy *proxy, guint flags, UkuiSettingsManager *manager);
static DBusGProxy* get_bus_proxy (DBusGConnection *connection);
static gboolean acquire_name_on_proxy (DBusGProxy *bus_proxy);
static void set_session_over_handler (DBusGConnection *bus, UkuiSettingsManager *manager);
static DBusHandlerResult bus_message_handler (DBusConnection *connection, DBusMessage* message, void* user_data);
static gboolean timed_exit_cb (void);

static void parse_args (int *argc, char ***argv);
static void debug_changed (GSettings *settings, gchar *key, gpointer user_data);
static void usd_log_default_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar* message, gpointer unused_data);

static gboolean   no_daemon    = TRUE;
static gboolean   replace      = FALSE;
static gboolean   debug        = FALSE;
static gboolean   do_timed_exit = FALSE;
static int        term_signal_pipe_fds[2];

static GOptionEntry entries[] = {
        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, ("Enable debugging code"), NULL },
        { "replace", 0, 0, G_OPTION_ARG_NONE, &replace, ("Replace the current daemon"), NULL },
        { "no-daemon", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &no_daemon, ("Don't become a daemon"), NULL },
        { "timed-exit", 0, 0, G_OPTION_ARG_NONE, &do_timed_exit, ("Exit after a time (for debugging)"), NULL },
        { NULL }
};

//using namespace std;

int main (int argc, char* argv[])
{
    UkuiSettingsManager*    manager = NULL;
    DBusGConnection*        bus = NULL;
    gboolean                res;
    GError*                 error = NULL;
    GSettings*              debug_settings = NULL;

    // ukui_settings_profile_start (NULL);

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

    // ukui_settings_profile_start ("opening gtk display");
    g_log_set_default_handler (usd_log_default_handler, NULL);

    bus = get_session_bus ();
    if (bus == NULL) {
        g_warning ("Could not get a connection to the bus");
        goto out;
    }

    if (! bus_register (bus)) {
        goto out;
    }

    // ukui_settings_profile_start ("ukui_settings_manager_new");
    manager = UkuiSettingsManager::ukuiSettingsManagerNew();
    // ukui_settings_profile_end ("ukui_settings_manager_new");
    if (manager == NULL) {
        g_warning ("Unable to register object");
        goto out;
    }

    set_session_over_handler (bus, manager);

    /* If we aren't started by dbus then load the plugins automatically.  Otherwise, wait for an Awake etc. */
    if (g_getenv ("DBUS_STARTER_BUS_TYPE") == NULL) {
        error = NULL;
        res = manager->ukuiSettingsManagerStart(&error);
        if (! res) {
            g_warning ("Unable to start: %s", error->message);
            g_error_free (error);
            goto out;
        }
    }
    if (do_timed_exit) {
        g_timeout_add_seconds (30, (GSourceFunc) timed_exit_cb, NULL);
    }

    //gtk_main ();

out:

    if (bus != NULL) dbus_g_connection_unref (bus);
    if (manager != NULL) g_object_unref (manager);
    if (debug_settings != NULL) g_object_unref (debug_settings);

    g_debug ("SettingsDaemon finished");

    //ukui_settings_profile_end (NULL);


    std::cout << "Hello World!" << std::endl;
    return app.exec();;
}

static void parse_args (int *argc, char ***argv)
{
    GError *error;
    GOptionContext *context;

    // ukui_settings_profile_start (NULL);

    context = g_option_context_new (NULL);

    g_option_context_add_main_entries (context, entries, NULL);
    //g_option_context_add_group (context, gtk_get_option_group (FALSE));

    error = NULL;
    if (!g_option_context_parse (context, argc, argv, &error)) {
        if (error != NULL) {
            g_warning ("%s", error->message);
            g_error_free (error);
        } else {
            g_warning ("Unable to initialize GTK+");
        }
        exit (EXIT_FAILURE);
    }

    g_option_context_free (context);
    //ukui_settings_profile_end (NULL);

    if (debug)
        g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
}

static void debug_changed (GSettings *settings, gchar *key, gpointer user_data)
{
        debug = g_settings_get_boolean (settings, DEBUG_KEY);
        if (debug) {
            g_warning ("Enable DEBUG");
            g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
        } else {
            g_warning ("Disable DEBUG");
            g_unsetenv ("G_MESSAGES_DEBUG");
        }
}

static void usd_log_default_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar* message, gpointer unused_data)
{
    /* filter out DEBUG messages if debug isn't set */
    if ((log_level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG && ! debug) return;
    g_log_default_handler (log_domain, log_level, message, unused_data);
}

static DBusGConnection* get_session_bus (void)
{
    GError          *error;
    DBusGConnection *bus;
    DBusConnection  *connection;

    error = NULL;
    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (bus == NULL) {
            g_warning ("Couldn't connect to session bus: %s",
                       error->message);
            g_error_free (error);
            goto out;
    }

    connection = dbus_g_connection_get_connection (bus);
    dbus_connection_add_filter (connection,
                                (DBusHandleMessageFunction)
                                bus_message_handler,
                                NULL, NULL);

    dbus_connection_set_exit_on_disconnect (connection, FALSE);

out:
    return bus;
}

static void on_session_over (DBusGProxy *proxy, UkuiSettingsManager *manager)
{
        /* not used, see on_session_end instead */
}

static void set_session_over_handler (DBusGConnection *bus, UkuiSettingsManager *manager)
{
    DBusGProxy *session_proxy;
    DBusGProxy *private_proxy;
    gchar *client_id = NULL;
    const char *startup_id;
    GError *error = NULL;
    gboolean res;

    g_assert (bus != NULL);

    //ukui_settings_profile_start (NULL);

    session_proxy = dbus_g_proxy_new_for_name (bus, UKUI_SESSION_DBUS_NAME, UKUI_SESSION_DBUS_OBJECT, UKUI_SESSION_DBUS_INTERFACE);

    dbus_g_object_register_marshaller(g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, G_TYPE_INVALID);

    dbus_g_proxy_add_signal(session_proxy, "SessionOver", G_TYPE_INVALID);

    // FIXME://
//    dbus_g_proxy_connect_signal(session_proxy, "SessionOver", G_CALLBACK (on_session_over), manager, NULL);

    /* Register with ukui-session */
    startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
    if (startup_id != NULL && *startup_id != '\0') {
        res = dbus_g_proxy_call (session_proxy,
                    "RegisterClient",
                     &error,
                     G_TYPE_STRING, "ukui-settings-daemon",
                     G_TYPE_STRING, startup_id,
                     G_TYPE_INVALID,
                     DBUS_TYPE_G_OBJECT_PATH, &client_id,
                     G_TYPE_INVALID);
    if (!res) {
        g_warning ("failed to register client '%s': %s", startup_id, error->message);
        g_error_free (error);
    } else {
        /* get org.gnome.SessionManager.ClientPrivate interface */
        private_proxy = dbus_g_proxy_new_for_name_owner (bus, UKUI_SESSION_DBUS_NAME,
                     client_id, UKUI_SESSION_PRIVATE_DBUS_INTERFACE,
                     &error);
        if (private_proxy == NULL) {
            g_warning ("DBUS error: %s", error->message);
            g_error_free (error);
        } else {
            /* get QueryEndSession */
            dbus_g_proxy_add_signal (private_proxy, "QueryEndSession", G_TYPE_UINT, G_TYPE_INVALID);
            dbus_g_proxy_connect_signal (private_proxy, "QueryEndSession",
                                         G_CALLBACK (on_session_query_end),
                                         manager, NULL);

            /* get EndSession */
            dbus_g_proxy_add_signal (private_proxy, "EndSession", G_TYPE_UINT, G_TYPE_INVALID);
            dbus_g_proxy_connect_signal (private_proxy, "EndSession",
                                         G_CALLBACK (on_session_end), manager, NULL);
        }

        g_free (client_id);
    }
    }

    // FIXME://
    //watch_for_term_signal (manager);
    //ukui_settings_profile_end (NULL);
}

static void on_session_query_end (DBusGProxy *proxy, guint flags, UkuiSettingsManager *manager)
{
        GError *error = NULL;
        gboolean ret = FALSE;

        /* send response */
        ret = dbus_g_proxy_call (proxy, "EndSessionResponse", &error,
                                 G_TYPE_BOOLEAN, TRUE /* ok */,
                                 G_TYPE_STRING, NULL /* reason */,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);
        if (!ret) {
                g_warning ("failed to send session response: %s", error->message);
                g_error_free (error);
        }
}

static void on_session_end (DBusGProxy *proxy, guint flags, UkuiSettingsManager *manager)
{
    GError *error = NULL;
    gboolean ret = FALSE;

    /* send response */
    ret = dbus_g_proxy_call (proxy, "EndSessionResponse", &error,
                 G_TYPE_BOOLEAN, TRUE /* ok */,
                 G_TYPE_STRING, NULL /* reason */,
                 G_TYPE_INVALID,
                 G_TYPE_INVALID);
    if (!ret) {
        g_warning ("failed to send session response: %s", error->message);
        g_error_free (error);
    }

    // FIXME://
//        ukui_settings_manager_stop (manager);
//        gtk_main_quit ();
}

static gboolean bus_register (DBusGConnection *bus)
{
    DBusGProxy      *bus_proxy;
    gboolean         ret;

    // ukui_settings_profile_start (NULL);

    ret = FALSE;

    bus_proxy = get_bus_proxy (bus);

    if (bus_proxy == NULL) {
            g_warning ("Could not construct bus_proxy object");
            goto out;
    }

    ret = acquire_name_on_proxy (bus_proxy);
    g_object_unref (bus_proxy);

    if (!ret) {
            g_warning ("Could not acquire name");
            goto out;
    }

    g_debug ("Successfully connected to D-Bus");

out:
    //ukui_settings_profile_end (NULL);

    return ret;
}

static DBusGProxy* get_bus_proxy (DBusGConnection *connection)
{
    DBusGProxy *bus_proxy;

    bus_proxy = dbus_g_proxy_new_for_name (connection,
                   DBUS_SERVICE_DBUS,
                   DBUS_PATH_DBUS,
                   DBUS_INTERFACE_DBUS);

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
    if (replace)
        flags |= DBUS_NAME_FLAG_REPLACE_EXISTING;

    error = NULL;
    res = dbus_g_proxy_call (bus_proxy,
                 "RequestName",
                 &error,
                 G_TYPE_STRING, USD_DBUS_NAME,
                 G_TYPE_UINT, flags,
                 G_TYPE_INVALID,
                 G_TYPE_UINT, &result,
                 G_TYPE_INVALID);
    if (! res) {
        if (error != NULL) {
            g_warning ("Failed to acquire %s: %s", USD_DBUS_NAME, error->message);
            g_error_free (error);
        } else {
            g_warning ("Failed to acquire %s", USD_DBUS_NAME);
        }
        goto out;
    }

    if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        if (error != NULL) {
                g_warning ("Failed to acquire %s: %s", USD_DBUS_NAME, error->message);
                g_error_free (error);
        } else {
                g_warning ("Failed to acquire %s", USD_DBUS_NAME);
        }
        goto out;
    }

    ret = TRUE;

out:
    return ret;
}

static DBusHandlerResult bus_message_handler (DBusConnection *connection, DBusMessage* message, void* user_data)
{
    if (dbus_message_is_signal (message,
                                DBUS_INTERFACE_LOCAL,
                                "Disconnected")) {
        // FIXME://
//            gtk_main_quit ();
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else if (dbus_message_is_signal (message,
                                     DBUS_INTERFACE_DBUS,
                                     "NameLost")) {
        g_warning ("D-Bus name lost, quitting");
        // FIXME://
//        gtk_main_quit ();
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean timed_exit_cb (void)
{
//    gtk_main_quit ();
    return FALSE;
}
