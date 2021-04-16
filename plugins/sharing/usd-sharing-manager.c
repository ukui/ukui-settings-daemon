/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
*
* Copyright (C) 2014 Bastien Nocera <hadess@hadess.net>
* Copyright (C) 2020 KylinSoft Co., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, see <http://www.gnu.org/licenses/>.
*
*/

#include "config.h"

#include <locale.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gstdio.h>

#include "ukui-settings-profile.h"
#include "usd-sharing-manager.h"

#define SHATINGS_SCHEMA     "org.ukui.SettingsDaemon.plugins.sharing"
#define KEY_SERVICE_NAME    "service-name"

typedef struct {
    const char  *name;
    GSettings   *settings;
} ServiceInfo;

struct _UsdSharingManager
{
    GObject                  parent;

    GDBusNodeInfo           *introspection_data;
    guint                    name_id;
    GDBusConnection         *connection;

    GCancellable            *cancellable;

    GHashTable              *services;

    char                    *current_network;
    char                    *current_network_name;
    char                    *carrier_type;
    UsdSharingStatus         sharing_status;
    GSettings               *services_settings;
};

#define USD_DBUS_NAME "org.ukui.SettingsDaemon"
#define USD_DBUS_PATH "/org/ukui/SettingsDaemon"
#define USD_DBUS_BASE_INTERFACE "org.ukui.SettingsDaemon"

#define USD_SHARING_DBUS_NAME USD_DBUS_NAME ".Sharing"
#define USD_SHARING_DBUS_PATH USD_DBUS_PATH "/Sharing"

static const gchar introspection_xml[] =
"<node>"
"  <interface name='org.ukui.SettingsDaemon.Sharing'>"
"    <annotation name='org.freedesktop.DBus.GLib.CSymbol' value='usd_sharing_manager'/>"
"    <property name='CurrentNetwork' type='s' access='read'/>"
"    <property name='CurrentNetworkName' type='s' access='read'/>"
"    <property name='CarrierType' type='s' access='read'/>"
"    <property name='SharingStatus' type='u' access='read'/>"
"    <method name='EnableService'>"
"      <arg name='service-name' direction='in' type='s'/>"
"    </method>"
"    <method name='DisableService'>"
"      <arg name='service-name' direction='in' type='s'/>"
"    </method>"
"    <method name='ListNetworks'>"
"      <arg name='service-name' direction='in' type='s'/>"
"      <arg name='networks' direction='out' type='a(sss)'/>"
"    </method>"
"  </interface>"
"</node>";

static void     usd_sharing_manager_class_init  (UsdSharingManagerClass *klass);
static void     usd_sharing_manager_init        (UsdSharingManager      *manager);
static void     usd_sharing_manager_finalize    (GObject                *object);

G_DEFINE_TYPE (UsdSharingManager, usd_sharing_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static const char * const services[] = {
    "vino-server"
};

static void
handle_unit_cb (GObject      *source_object,
            GAsyncResult *res,
            gpointer      user_data)
{
    GError *error = NULL;
    GVariant *ret;
    const char *operation = user_data;

    ret = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object),
                                         res, &error);
    if (!ret) {
            g_autofree gchar *remote_error = g_dbus_error_get_remote_error (error);

            if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
                g_strcmp0 (remote_error, "org.freedesktop.systemd1.NoSuchUnit") != 0)
                    g_warning ("Failed to %s service: %s", operation, error->message);
            g_error_free (error);
            return;
    }

    g_variant_unref (ret);

}

static void
usd_sharing_manager_handle_service (UsdSharingManager   *manager,
                                const char          *method,
                                ServiceInfo         *service)
{
    char *service_file;

    service_file = g_strdup_printf ("%s.service", service->name);
    g_dbus_connection_call (manager->connection,
                            "org.freedesktop.systemd1",
                            "/org/freedesktop/systemd1",
                            "org.freedesktop.systemd1.Manager",
                            method,
                            g_variant_new ("(ss)", service_file, "replace"),
                            NULL,
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            manager->cancellable,
                            handle_unit_cb,
                            (gpointer) method);

    g_free (service_file);
}

static void
usd_sharing_manager_start_service (UsdSharingManager *manager,
                               ServiceInfo       *service)
{
    g_debug ("About to start %s", service->name);

    /* We use StartUnit, not StartUnitReplace, since the latter would
     * cancel any pending start we already have going from an
     * earlier _start_service() call */
    usd_sharing_manager_handle_service (manager, "StartUnit", service);
}

static void
usd_sharing_manager_stop_service (UsdSharingManager *manager,
                              ServiceInfo       *service)
{
    g_debug ("About to stop %s", service->name);

    usd_sharing_manager_handle_service (manager, "StopUnit", service);
}

static gboolean
service_is_enabled_on_current_connection (UsdSharingManager *manager,
                                      ServiceInfo       *service)
{
    return TRUE;//FALSE;
}

static void
usd_sharing_manager_sync_services (UsdSharingManager *manager)
{
    GList *services, *l;

    services = g_hash_table_get_values (manager->services);

    for (l = services; l != NULL; l = l->next) {
            ServiceInfo *service = l->data;
            gboolean should_be_started = FALSE;
            if (manager->sharing_status == USD_SHARING_STATUS_AVAILABLE &&
                service_is_enabled_on_current_connection (manager, service))
                    should_be_started = TRUE;

            if (should_be_started)
                    usd_sharing_manager_start_service (manager, service);
            else
                    usd_sharing_manager_stop_service (manager, service);
    }
    g_list_free (services);
}

static char **
get_connections_for_service (UsdSharingManager *manager,
                         const char        *service_name)
{
    const char * const * connections [] = { NULL };
    return g_strdupv ((char **) connections);
}

static gboolean
check_service (UsdSharingManager  *manager,
           const char         *service_name,
           GError            **error)
{
    if (g_hash_table_lookup (manager->services, service_name))
            return TRUE;

    g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                 "Invalid service name '%s'", service_name);
    return FALSE;
}

static gboolean
usd_sharing_manager_enable_service (UsdSharingManager  *manager,
                                const char         *service_name,
                                GError            **error)
{
    ServiceInfo *service;
    char **service_list;
    GPtrArray *array;
    guint i;

//    if (!check_service (manager, service_name, error))
//            return FALSE;

    if (manager->sharing_status != USD_SHARING_STATUS_AVAILABLE) {
            g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                         "Sharing cannot be enabled on this network, status is '%d'", manager->sharing_status);
            return FALSE;
    }

    service = g_hash_table_lookup (manager->services, service_name);
    if (!service){
        service = g_new0 (ServiceInfo, 1);
        service->name = g_strdup (service_name);//services[i];
        g_hash_table_insert (manager->services, (gpointer) service_name, service);
    }
    service_list = g_settings_get_strv (manager->services_settings, KEY_SERVICE_NAME);
    array = g_ptr_array_new ();
    for (i = 0; service_list[i] != NULL; i++) {
            if (g_strcmp0 (service_list[i], service_name) == 0)
                    goto bail;
            g_ptr_array_add (array, service_list[i]);
    }
    g_ptr_array_add (array, service_name);
    g_ptr_array_add (array, NULL);

    g_settings_set_strv (manager->services_settings, KEY_SERVICE_NAME, (const gchar *const *) array->pdata);

bail:

    usd_sharing_manager_start_service (manager, service);

    g_ptr_array_unref (array);
    g_strfreev (service_list);

    return TRUE;
}

static gboolean
usd_sharing_manager_disable_service (UsdSharingManager  *manager,
                                 const char         *service_name,
                                 GError            **error)
{
    ServiceInfo *service;
    char **service_list;
    GPtrArray *array;
    guint i;

    // if (!check_service (manager, service_name, error))
    //         return FALSE;

    service = g_hash_table_lookup (manager->services, service_name);
    if (!service){
        service = g_new0 (ServiceInfo, 1);
        service->name = g_strdup (service_name);
        g_hash_table_insert (manager->services, (gpointer) service_name, service);
    }
    service_list = g_settings_get_strv (manager->services_settings, KEY_SERVICE_NAME);
    array = g_ptr_array_new ();

    for (i = 0; service_list[i] != NULL; i++) {
            if (g_strcmp0 (service_list[i], service_name) == 0)
                continue;
            g_ptr_array_add (array, service_list[i]);
    }

    g_ptr_array_add (array, NULL);
    g_settings_set_strv (manager->services_settings, KEY_SERVICE_NAME, (const gchar *const *) array->pdata);

    g_ptr_array_unref (array);
    g_strfreev (service_list);

    usd_sharing_manager_stop_service (manager, service);

    return TRUE;
}

static const char *
get_type_and_name_for_connection_uuid (UsdSharingManager *manager,
                                   const char        *id,
                                   const char       **name)
{
    return NULL;
}

static GVariant *
usd_sharing_manager_list_networks (UsdSharingManager  *manager,
                               const char         *service_name,
                               GError            **error)
{
    char **connections;
    GVariantBuilder builder;
    guint i;

    if (!check_service (manager, service_name, error))
            return NULL;

    connections = get_connections_for_service (manager, service_name);

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("(a(sss))"));
    g_variant_builder_open (&builder, G_VARIANT_TYPE ("a(sss)"));

    for (i = 0; connections[i] != NULL; i++) {
            const char *type, *name;

            type = get_type_and_name_for_connection_uuid (manager, connections[i], &name);
            if (!type)
                    continue;

            g_variant_builder_add (&builder, "(sss)", connections[i], name, type);
    }
    g_strfreev (connections);

    g_variant_builder_close (&builder);

    return g_variant_builder_end (&builder);
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                 const gchar     *sender,
                 const gchar     *object_path,
                 const gchar     *interface_name,
                 const gchar     *property_name,
                 GError         **error,
                 gpointer         user_data)
{
    UsdSharingManager *manager = USD_SHARING_MANAGER (user_data);

    /* Check session pointer as a proxy for whether the manager is in the
       start or stop state */
    if (manager->connection == NULL)
            return NULL;

    if (g_strcmp0 (property_name, "CurrentNetwork") == 0) {
            return g_variant_new_string (manager->current_network);
    }

    if (g_strcmp0 (property_name, "CurrentNetworkName") == 0) {
            return g_variant_new_string (manager->current_network_name);
    }

    if (g_strcmp0 (property_name, "CarrierType") == 0) {
            return g_variant_new_string (manager->carrier_type);
    }

    if (g_strcmp0 (property_name, "SharingStatus") == 0) {
            return g_variant_new_uint32 (manager->sharing_status);
    }

    return NULL;
}

static void
handle_method_call (GDBusConnection       *connection,
                const gchar           *sender,
                const gchar           *object_path,
                const gchar           *interface_name,
                const gchar           *method_name,
                GVariant              *parameters,
                GDBusMethodInvocation *invocation,
                gpointer               user_data)
{
    UsdSharingManager *manager = (UsdSharingManager *) user_data;

    g_debug ("Calling method '%s' for sharing", method_name);

    /* Check session pointer as a proxy for whether the manager is in the
       start or stop state */
    if (manager->connection == NULL)
            return;

    if (g_strcmp0 (method_name, "EnableService") == 0) {
            const char *service;
            GError *error = NULL;

            g_variant_get (parameters, "(&s)", &service);
            if (!usd_sharing_manager_enable_service (manager, service, &error))
                    g_dbus_method_invocation_take_error (invocation, error);
            else
                    g_dbus_method_invocation_return_value (invocation, NULL);
    } else if (g_strcmp0 (method_name, "DisableService") == 0) {
            const char *service;
            GError *error = NULL;

            g_variant_get (parameters, "(&s)", &service);
            if (!usd_sharing_manager_disable_service (manager, service, &error))
                    g_dbus_method_invocation_take_error (invocation, error);
            else
                    g_dbus_method_invocation_return_value (invocation, NULL);
    } else if (g_strcmp0 (method_name, "ListNetworks") == 0) {
            const char *service;
            GError *error = NULL;
            GVariant *variant;

            g_variant_get (parameters, "(&s)", &service);
            variant = usd_sharing_manager_list_networks (manager, service, &error);
            if (!variant)
                    g_dbus_method_invocation_take_error (invocation, error);
            else
                    g_dbus_method_invocation_return_value (invocation, variant);
    }
}

static const GDBusInterfaceVTable interface_vtable =
{
    handle_method_call,
    handle_get_property,
    NULL
};

static void
on_bus_gotten (GObject               *source_object,
           GAsyncResult          *res,
           UsdSharingManager     *manager)
{
    GDBusConnection *connection;
    GError *error = NULL;

    connection = g_bus_get_finish (res, &error);
    if (connection == NULL) {
            if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                    g_warning ("Could not get session bus: %s", error->message);
            g_error_free (error);
            return;
    }
    manager->connection = connection;

    g_dbus_connection_register_object (connection,
                                       USD_SHARING_DBUS_PATH,
                                       manager->introspection_data->interfaces[0],
                                       &interface_vtable,
                                       manager,
                                       NULL,
                                       NULL);

    manager->name_id = g_bus_own_name_on_connection (connection,
                                                           USD_SHARING_DBUS_NAME,
                                                           G_BUS_NAME_OWNER_FLAGS_NONE,
                                                           NULL,
                                                           NULL,
                                                           NULL,
                                                           NULL);
    manager->sharing_status = USD_SHARING_STATUS_AVAILABLE;
    usd_sharing_manager_sync_services (manager);
}

#define RYGEL_BUS_NAME "org.ukui.Rygel1"
#define RYGEL_OBJECT_PATH "/org/ukui/Rygel1"
#define RYGEL_INTERFACE_NAME "org.ukui.Rygel1"

static void
usd_sharing_manager_disable_rygel (void)
{
    GDBusConnection *connection;
    gchar *path;

    path = g_build_filename (g_get_user_config_dir (), "autostart",
                             "rygel.desktop", NULL);
    if (!g_file_test (path, G_FILE_TEST_IS_SYMLINK | G_FILE_TEST_IS_REGULAR))
            goto out;

    g_unlink (path);

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
    if (connection) {
            g_dbus_connection_call (connection, RYGEL_BUS_NAME, RYGEL_OBJECT_PATH, RYGEL_INTERFACE_NAME,
                                    "Shutdown", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                                    NULL, NULL, NULL);
    }
    g_object_unref (connection);

out:
    g_free (path);
}

gboolean
usd_sharing_manager_start (UsdSharingManager *manager,
                       GError           **error)
{
    g_debug ("Starting sharing manager");
    ukui_settings_profile_start (NULL);

    manager->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    g_assert (manager->introspection_data != NULL);

    usd_sharing_manager_disable_rygel ();

    manager->cancellable = g_cancellable_new ();
    /* Start process of owning a D-Bus name */
    g_bus_get (G_BUS_TYPE_SESSION,
               manager->cancellable,
               (GAsyncReadyCallback) on_bus_gotten,
               manager);

    ukui_settings_profile_end (NULL);
    return TRUE;
}

void
usd_sharing_manager_stop (UsdSharingManager *manager)
{
    g_debug ("Stopping sharing manager");

    if (manager->sharing_status == USD_SHARING_STATUS_AVAILABLE &&
        manager->connection != NULL) {
            manager->sharing_status = USD_SHARING_STATUS_OFFLINE;
            usd_sharing_manager_sync_services (manager);
    }

    if (manager->cancellable) {
            g_cancellable_cancel (manager->cancellable);
            g_clear_object (&manager->cancellable);
    }

    if (manager->name_id != 0) {
            g_bus_unown_name (manager->name_id);
            manager->name_id = 0;
    }

    g_object_unref (manager->services_settings);
    g_clear_pointer (&manager->introspection_data, g_dbus_node_info_unref);
    g_clear_object (&manager->connection);

    g_clear_pointer (&manager->current_network, g_free);
    g_clear_pointer (&manager->current_network_name, g_free);
    g_clear_pointer (&manager->carrier_type, g_free);
}

static void
usd_sharing_manager_class_init (UsdSharingManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = usd_sharing_manager_finalize;
}

static void
service_free (gpointer pointer)
{
    ServiceInfo *service = pointer;

    g_free (service);
}

static void
usd_sharing_manager_init (UsdSharingManager *manager)
{
    guint i;
    gchar **services_list;

    manager->services_settings = g_settings_new (SHATINGS_SCHEMA);
    services_list = g_settings_get_strv (manager->services_settings, KEY_SERVICE_NAME);
    manager->services = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, service_free);

    /* Default state */
    manager->current_network = g_strdup ("");
    manager->current_network_name = g_strdup ("");
    manager->carrier_type = g_strdup ("");
    manager->sharing_status = USD_SHARING_STATUS_OFFLINE;

    for (i = 0; i <= G_N_ELEMENTS (services_list); i++) {
            ServiceInfo *service;

            if (!services_list[i])
                continue;
            service = g_new0 (ServiceInfo, 1);
            service->name = g_strdup (services_list[i]);//services[i];

            g_hash_table_insert (manager->services, (gpointer) services_list[i], service);
    }
    g_strfreev (services_list);
}

static void
usd_sharing_manager_finalize (GObject *object)
{
    UsdSharingManager *manager;

    g_return_if_fail (object != NULL);
    g_return_if_fail (USD_IS_SHARING_MANAGER (object));

    manager = USD_SHARING_MANAGER (object);

    g_return_if_fail (manager != NULL);

    usd_sharing_manager_stop (manager);

    g_hash_table_unref (manager->services);

    G_OBJECT_CLASS (usd_sharing_manager_parent_class)->finalize (object);
}

UsdSharingManager *
usd_sharing_manager_new (void)
{
    if (manager_object != NULL) {
            g_object_ref (manager_object);
    } else {
            manager_object = g_object_new (USD_TYPE_SHARING_MANAGER, NULL);
            g_object_add_weak_pointer (manager_object,
                                       (gpointer *) &manager_object);
    }

    return USD_SHARING_MANAGER (manager_object);
}
