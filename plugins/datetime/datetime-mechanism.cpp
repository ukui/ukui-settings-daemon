/*#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>*/

#include <dbus/dbus-glib-lowlevel.h>
#include <sys/time.h>
#include <errno.h>

#include "system-timezone.h"
#include "datetime-mechanism.h"
#include "clib-syslog.h"

DatetimeMechanism::DatetimeMechanism()
{
    //dbus_g_object_type_install_info (USD_DATETIME_TYPE_MECHANISM, &dbus_glib_usd_datetime_mechanism_object_info);
    dbus_g_error_domain_register (USD_DATETIME_MECHANISM_ERROR, NULL, USD_DATETIME_MECHANISM_TYPE_ERROR);
}

DatetimeMechanism::~DatetimeMechanism()
{
    g_object_unref (system_bus_proxy);
}
//这个代码好像不需要单实例
DatetimeMechanism* DatetimeMechanism::DatetimeMechanismNew()
{
    bool res;
    DatetimeMechanism* mechanism=new DatetimeMechanism();
    res = register_mechanism (mechanism);
    if (!res)
            return NULL;
    return mechanism;
}

static gboolean
do_exit (gpointer user_data)
{
        CT_SYSLOG(LOG_DEBUG,"Exiting due to inactivity");
        exit (1);
        return FALSE;
}

void reset_killtimer (void)
{
        static guint timer_id = 0;

        if (timer_id > 0) {
                g_source_remove (timer_id);
        }
        CT_SYSLOG(LOG_DEBUG,"Setting killtimer to 30 seconds...");
        timer_id = g_timeout_add_seconds (30, do_exit, NULL);
}

GQuark
usd_datetime_mechanism_error_quark (void)
{
        static GQuark ret = 0;

        if (ret == 0) {
                ret = g_quark_from_static_string ("usd_datetime_mechanism_error");
        }

        return ret;
}


#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
usd_datetime_mechanism_error_get_type (void)
{
        static GType etype = 0;
        
        if (etype == 0)
        {
            static const GEnumValue values[] ={
                    ENUM_ENTRY (USD_DATETIME_MECHANISM_ERROR_GENERAL, "GeneralError"),
                    ENUM_ENTRY (USD_DATETIME_MECHANISM_ERROR_NOT_PRIVILEGED, "NotPrivileged"),
                    ENUM_ENTRY (USD_DATETIME_MECHANISM_ERROR_INVALID_TIMEZONE_FILE, "InvalidTimezoneFile"),
                    { 0, 0, 0 }
            };

            g_assert (USD_DATETIME_MECHANISM_NUM_ERRORS == G_N_ELEMENTS (values) - 1);

            etype = g_enum_register_static ("UsdDatetimeMechanismError", values);
        }
        
        return etype;
}

gboolean
register_mechanism (DatetimeMechanism *mechanism)
{
        GError *error = NULL;

        mechanism->auth = polkit_authority_get_sync (NULL, &error);
        if (mechanism->auth == NULL) {
            if (error != NULL) {
                    g_critical ("error getting system bus: %s", error->message);
                    g_error_free (error);
            }
            goto error;
        }

        mechanism->system_bus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (mechanism->system_bus_connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                goto error;
        }
        //TODO... 类指针转为Gobject,如果不继承于Gobject,难保不出问题吧?
        //或者替换为QDBus
        dbus_g_connection_register_g_object (mechanism->system_bus_connection, "/",
                                             G_OBJECT (mechanism));

        mechanism->system_bus_proxy =
                dbus_g_proxy_new_for_name (mechanism->system_bus_connection,
                                          DBUS_SERVICE_DBUS,
                                          DBUS_PATH_DBUS,
                                          DBUS_INTERFACE_DBUS);

        reset_killtimer ();
        return TRUE;
error:
        return FALSE;
}

gboolean
_check_polkit_for_action (DatetimeMechanism *mechanism, DBusGMethodInvocation *context, const char *action)
{
        const char *sender;
        GError *error;
        PolkitSubject *subject;
        PolkitAuthorizationResult *result;

        error = NULL;

        /* Check that caller is privileged */
        sender = dbus_g_method_get_sender (context);
        subject = polkit_system_bus_name_new (sender);

        result = polkit_authority_check_authorization_sync (mechanism->auth,
                                                            subject,
                                                            action,
                                                            NULL,
                                                            POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                                                            NULL, &error);
        g_object_unref (subject);

        if (error) {
            dbus_g_method_return_error (context, error);
            g_error_free (error);

            return FALSE;
        }

        if (!polkit_authorization_result_get_is_authorized (result)) {
            error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                 USD_DATETIME_MECHANISM_ERROR_NOT_PRIVILEGED,
                                 "Not Authorized for action %s", action);
            dbus_g_method_return_error (context, error);
            g_error_free (error);
            g_object_unref (result);

            return FALSE;
        }

        g_object_unref (result);

        return TRUE;
}


gboolean
_set_time (DatetimeMechanism  *mechanism,
           const struct timeval  *tv,
           DBusGMethodInvocation *context)
{
        GError *error;

        if (!_check_polkit_for_action (mechanism, context, "org.ukui.settingsdaemon.datetimemechanism.settime"))
                return FALSE;

        if (settimeofday (tv, NULL) != 0) {
            error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                 USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                 "Error calling settimeofday({%lld,%lld}): %s",
                                 (gint64) tv->tv_sec, (gint64) tv->tv_usec,
                                 strerror (errno));
            dbus_g_method_return_error (context, error);
            g_error_free (error);
            return FALSE;
        }

        if (g_file_test ("/sbin/hwclock", 
                         GFileTest(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_EXECUTABLE))) {
            int exit_status;
            if (!g_spawn_command_line_sync ("/sbin/hwclock --systohc", NULL, NULL, &exit_status, &error)) {
                    GError *error2;
                    error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                          USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                          "Error spawning /sbin/hwclock: %s", error->message);
                    g_error_free (error);
                    dbus_g_method_return_error (context, error2);
                    g_error_free (error2);
                    return FALSE;
            }
            if (WEXITSTATUS (exit_status) != 0) {
                    error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                         USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                         "/sbin/hwclock returned %d", exit_status);
                    dbus_g_method_return_error (context, error);
                    g_error_free (error);
                    return FALSE;
            }
        }

        dbus_g_method_return (context);
        return TRUE;
}

static gboolean
_rh_update_etc_sysconfig_clock (DBusGMethodInvocation *context, const char *key, const char *value)
{
    /* On Red Hat / Fedora, the /etc/sysconfig/clock file needs to be kept in sync */
    if (g_file_test ("/etc/sysconfig/clock", GFileTest(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        char **lines;
        int n;
        gboolean replaced;
        char *data;
        gsize len;
        GError *error;

        error = NULL;

        if (!g_file_get_contents ("/etc/sysconfig/clock", &data, &len, &error)) {
                GError *error2;
                error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                      USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                      "Error reading /etc/sysconfig/clock file: %s", error->message);
                g_error_free (error);
                dbus_g_method_return_error (context, error2);
                g_error_free (error2);
                return FALSE;
        }
        replaced = FALSE;
        lines = g_strsplit (data, "\n", 0);
        g_free (data);

        for (n = 0; lines[n] != NULL; n++) {
                if (g_str_has_prefix (lines[n], key)) {
                        g_free (lines[n]);
                        lines[n] = g_strdup_printf ("%s%s", key, value);
                        replaced = TRUE;
                }
        }
        if (replaced) {
            GString *str;

            str = g_string_new (NULL);
            for (n = 0; lines[n] != NULL; n++) {
                g_string_append (str, lines[n]);
                if (lines[n + 1] != NULL)
                    g_string_append_c (str, '\n');
            }
            data = g_string_free (str, FALSE);
            len = strlen (data);
            if (!g_file_set_contents ("/etc/sysconfig/clock", data, len, &error)) {
                GError *error2;
                error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                      USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                      "Error updating /etc/sysconfig/clock: %s", error->message);
                g_error_free (error);
                dbus_g_method_return_error (context, error2);
                g_error_free (error2);
                g_free (data);
                return FALSE;
            }
            g_free (data);
        }
        g_strfreev (lines);
    }

    return TRUE;
}

/* exported methods */
gboolean
usd_datetime_mechanism_set_time (DatetimeMechanism  *mechanism,
                                 gint64                 seconds_since_epoch,
                                 DBusGMethodInvocation *context)
{
    struct timeval tv;

    reset_killtimer ();
    CT_SYSLOG(LOG_DEBUG,"SetTime(%lld) called", seconds_since_epoch);

    tv.tv_sec = (time_t) seconds_since_epoch;
    tv.tv_usec = 0;
    return _set_time (mechanism, &tv, context);
}

gboolean
usd_datetime_mechanism_adjust_time (DatetimeMechanism  *mechanism,
                                    gint64                 seconds_to_add,
                                    DBusGMethodInvocation *context)
{
    struct timeval tv;

    reset_killtimer ();
    CT_SYSLOG(LOG_DEBUG,"AdjustTime(%lld) called", seconds_to_add);

    if (gettimeofday (&tv, NULL) != 0) {
        GError *error;
        error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                             USD_DATETIME_MECHANISM_ERROR_GENERAL,
                             "Error calling gettimeofday(): %s", strerror (errno));
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return FALSE;
    }

    tv.tv_sec += (time_t) seconds_to_add;
    return _set_time (mechanism, &tv, context);
}


gboolean
usd_datetime_mechanism_set_timezone (DatetimeMechanism  *mechanism,
                                     const char            *zone_file,
                                     DBusGMethodInvocation *context)
{
    GError *error;

    reset_killtimer ();
    CT_SYSLOG(LOG_DEBUG,"SetTimezone('%s') called", zone_file);

    if (!_check_polkit_for_action (mechanism, context, "org.ukui.settingsdaemon.datetimemechanism.settimezone"))
            return FALSE;

    error = NULL;

    if (!system_timezone_set_from_file (zone_file, &error)) {
        GError *error2;
        int     code;

        if (error->code == SYSTEM_TIMEZONE_ERROR_INVALID_TIMEZONE_FILE)
            code = USD_DATETIME_MECHANISM_ERROR_INVALID_TIMEZONE_FILE;
        else
            code = USD_DATETIME_MECHANISM_ERROR_GENERAL;

        error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                              code, "%s", error->message);

        g_error_free (error);

        dbus_g_method_return_error (context, error2);
        g_error_free (error2);

        return FALSE;
    }

    dbus_g_method_return (context);
    return TRUE;
}


gboolean
usd_datetime_mechanism_get_timezone (DatetimeMechanism   *mechism,
                                     DBusGMethodInvocation  *context)
{
    char *timezone;

    reset_killtimer ();

    timezone = system_timezone_find ();

    dbus_g_method_return (context, timezone);

    return TRUE;
}

gboolean
usd_datetime_mechanism_get_hardware_clock_using_utc (DatetimeMechanism  *mechanism,
                                                     DBusGMethodInvocation *context)
{
    char **lines;
    char *data;
    gsize len;
    GError *error;
    gboolean is_utc;

    error = NULL;

    if (!g_file_get_contents ("/etc/adjtime", &data, &len, &error)) {
        GError *error2;
        error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                              USD_DATETIME_MECHANISM_ERROR_GENERAL,
                              "Error reading /etc/adjtime file: %s", error->message);
        g_error_free (error);
        dbus_g_method_return_error (context, error2);
        g_error_free (error2);
        return FALSE;
    }

    lines = g_strsplit (data, "\n", 0);
    g_free (data);

    if (g_strv_length (lines) < 3) {
        error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                             USD_DATETIME_MECHANISM_ERROR_GENERAL,
                             "Cannot parse /etc/adjtime");
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        g_strfreev (lines);
        return FALSE;
    }

    if (strcmp (lines[2], "UTC") == 0) {
            is_utc = TRUE;
    } else if (strcmp (lines[2], "LOCAL") == 0) {
            is_utc = FALSE;
    } else {
            error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                 USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                 "Expected UTC or LOCAL at line 3 of /etc/adjtime; found '%s'", lines[2]);
            dbus_g_method_return_error (context, error);
            g_error_free (error);
            g_strfreev (lines);
            return FALSE;
    }
    g_strfreev (lines);
    dbus_g_method_return (context, is_utc);
    return TRUE;
}

gboolean
usd_datetime_mechanism_set_hardware_clock_using_utc (DatetimeMechanism  *mechanism,
                                                     gboolean               using_utc,
                                                     DBusGMethodInvocation *context)
{
    GError *error;
    error = NULL;

    if (!_check_polkit_for_action (mechanism, context,
                                   "org.ukui.settingsdaemon.datetimemechanism.configurehwclock"))
        return FALSE;

    if (g_file_test ("/sbin/hwclock",
                     GFileTest(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_EXECUTABLE))) {
        int exit_status;
        char *cmd;
        cmd = g_strdup_printf ("/sbin/hwclock %s --systohc", using_utc ? "--utc" : "--localtime");
        if (!g_spawn_command_line_sync (cmd, NULL, NULL, &exit_status, &error)) {
            GError *error2;
            error2 = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                  USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                  "Error spawning /sbin/hwclock: %s", error->message);
            g_error_free (error);
            dbus_g_method_return_error (context, error2);
            g_error_free (error2);
            g_free (cmd);
            return FALSE;
        }
        g_free (cmd);
        if (WEXITSTATUS (exit_status) != 0) {
            error = g_error_new (USD_DATETIME_MECHANISM_ERROR,
                                 USD_DATETIME_MECHANISM_ERROR_GENERAL,
                                 "/sbin/hwclock returned %d", exit_status);
            dbus_g_method_return_error (context, error);
            g_error_free (error);
            return FALSE;
        }

        if (!_rh_update_etc_sysconfig_clock (context, "UTC=", using_utc ? "true" : "false"))
            return FALSE;

    }
    dbus_g_method_return (context);
    return TRUE;
}

void
check_can_do (DatetimeMechanism  *mechanism,
              const char            *action,
              DBusGMethodInvocation *context)
{
    const char *sender;
    PolkitSubject *subject;
    PolkitAuthorizationResult *result;
    GError *error;

    /* Check that caller is privileged */
    sender = dbus_g_method_get_sender (context);
    subject = polkit_system_bus_name_new (sender);

    error = NULL;
    result = polkit_authority_check_authorization_sync (mechanism->auth,
                                                        subject,
                                                        action,
                                                        NULL,
                                                        (PolkitCheckAuthorizationFlags)0,
                                                        NULL,
                                                        &error);
    g_object_unref (subject);

    if (error) {
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (polkit_authorization_result_get_is_authorized (result)) {
        dbus_g_method_return (context, 2);
    }
    else if (polkit_authorization_result_get_is_challenge (result)) {
        dbus_g_method_return (context, 1);
    }
    else {
        dbus_g_method_return (context, 0);
    }

    g_object_unref (result);
}


gboolean
usd_datetime_mechanism_can_set_time (DatetimeMechanism  *mechanism,
                                     DBusGMethodInvocation *context)
{
    check_can_do (mechanism,
                  "org.ukui.settingsdaemon.datetimemechanism.settime",
                  context);
    return TRUE;
}

gboolean
usd_datetime_mechanism_can_set_timezone (DatetimeMechanism  *mechanism,
                                         DBusGMethodInvocation *context)
{
    check_can_do (mechanism,
                  "org.ukui.settingsdaemon.datetimemechanism.settimezone",
                  context);
    return TRUE;
}
