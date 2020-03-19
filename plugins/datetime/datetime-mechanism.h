#ifndef DATETIME_MECHANISM_H
#define DATETIME_MECHANISM_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <polkit/polkit.h>  //for PolkitAuthority

class DatetimeMechanism{
public:
    ~DatetimeMechanism();
    static DatetimeMechanism* DatetimeMechanismNew();

private:
    DatetimeMechanism();
    friend gboolean register_mechanism (DatetimeMechanism *mechanism);
    friend gboolean _check_polkit_for_action (DatetimeMechanism *mechanism, DBusGMethodInvocation *context, const char *action);
    friend gboolean _set_time (DatetimeMechanism  *mechanism,
                               const struct timeval  *tv,
                               DBusGMethodInvocation *context);
    friend void check_can_do (DatetimeMechanism  *mechanism,
                              const char            *action,
                              DBusGMethodInvocation *context);

    DBusGConnection *system_bus_connection;
    DBusGProxy      *system_bus_proxy;
    PolkitAuthority *auth;
};

typedef enum
{
        USD_DATETIME_MECHANISM_ERROR_GENERAL,
        USD_DATETIME_MECHANISM_ERROR_NOT_PRIVILEGED,
        USD_DATETIME_MECHANISM_ERROR_INVALID_TIMEZONE_FILE,
        USD_DATETIME_MECHANISM_NUM_ERRORS
} UsdDatetimeMechanismError;

#define USD_DATETIME_MECHANISM_ERROR usd_datetime_mechanism_error_quark ()

GType usd_datetime_mechanism_error_get_type (void);
#define USD_DATETIME_MECHANISM_TYPE_ERROR (usd_datetime_mechanism_error_get_type ())


GQuark    usd_datetime_mechanism_error_quark(void);

/* exported methods */
gboolean  usd_datetime_mechanism_get_timezone (DatetimeMechanism   *mechanism,
                                                         DBusGMethodInvocation  *context);
gboolean  usd_datetime_mechanism_set_timezone (DatetimeMechanism   *mechanism,
                                                         const char             *zone_file,
                                                         DBusGMethodInvocation  *context);

gboolean  usd_datetime_mechanism_can_set_timezone (DatetimeMechanism  *mechanism,
                                                             DBusGMethodInvocation *context);

gboolean  usd_datetime_mechanism_set_time     (DatetimeMechanism  *mechanism,
                                                         gint64                 seconds_since_epoch,
                                                         DBusGMethodInvocation *context);

gboolean  usd_datetime_mechanism_can_set_time (DatetimeMechanism  *mechanism,
                                                         DBusGMethodInvocation *context);

gboolean  usd_datetime_mechanism_adjust_time  (DatetimeMechanism  *mechanism,
                                                         gint64                 seconds_to_add,
                                                         DBusGMethodInvocation *context);

gboolean  usd_datetime_mechanism_get_hardware_clock_using_utc  (DatetimeMechanism  *mechanism,
                                                                          DBusGMethodInvocation *context);

gboolean  usd_datetime_mechanism_set_hardware_clock_using_utc  (DatetimeMechanism  *mechanism,
                                                                          gboolean               using_utc,
                                                                          DBusGMethodInvocation *context);

#endif /* DATETIME_MECHANISM_H */
