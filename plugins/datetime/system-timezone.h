#ifndef SYSTEM_TIMEZONE_H
#define SYSTEM_TIMEZONE_H

//#include <QObject>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#ifdef HAVE_SOLARIS
#define SYSTEM_ZONEINFODIR "/usr/share/lib/zoneinfo/tab"
#else
#define SYSTEM_ZONEINFODIR "/usr/share/zoneinfo"
#endif

#define CHECK_NB 5

class SystemTimezone /*: QObject*/{
//    Q_OBJECT
public:
    ~SystemTimezone();
    static SystemTimezone* SystemTimezoneNew();
    void SystemTimezoneStop();

    void (* changed) (SystemTimezone *systz,const char *tz);

//Q_SIGNALS:
    void monitorChanged(char*);
private:
    SystemTimezone(/*QObject* parent=0*/);
    void system_timezone_constructor ();
    friend const char *system_timezone_get (SystemTimezone *systz);
    friend const char *system_timezone_get_env (SystemTimezone *systz);
    static void system_timezone_monitor_changed (GFileMonitor *handle,
                                     GFile *file,
                                     GFile *other_file,
                                     GFileMonitorEvent event,
                                     gpointer user_data);


    static SystemTimezone* mSystemTimezone;
    char *tz;
    char *env_tz;
    GFileMonitor *monitors[CHECK_NB];
};

/* Functions to set the timezone. They won't be used by the applet, but
 * by a program with more privileges */

#define SYSTEM_TIMEZONE_ERROR system_timezone_error_quark ()
GQuark system_timezone_error_quark (void);

typedef enum
{
        SYSTEM_TIMEZONE_ERROR_GENERAL,
        SYSTEM_TIMEZONE_ERROR_INVALID_TIMEZONE_FILE,
        SYSTEM_TIMEZONE_NUM_ERRORS
} SystemTimezoneError;

char *system_timezone_find (void);

gboolean system_timezone_set_from_file (const char  *zone_file,
                                        GError     **error);
gboolean system_timezone_set (const char  *tz,
                              GError     **error);


#endif /* SYSTEM_TIMEZONE_H */
