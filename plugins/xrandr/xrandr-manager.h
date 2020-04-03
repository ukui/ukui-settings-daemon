#ifndef XRANDRMANAGER_H
#define XRANDRMANAGER_H

#include <QObject>
#include <QDBusConnectionInterface>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr.h>
#include <libmate-desktop/mate-rr-config.h>
#include <libmate-desktop/mate-rr-labeler.h>
#include <libmate-desktop/mate-desktop-utils.h>

#define USD_DBUS_PATH "/org/ukui/SettingsDaemon"
#define USD_DBUS_NAME "org.ukui.SettingsDaemon"
#define USD_XRANDR_DBUS_PATH USD_DBUS_PATH "/XRANDR"
#define USD_XRANDR_DBUS_NAME USD_DBUS_NAME ".XRANDR"

namespace UsdXrandr{
class XrandrManager;
}

typedef  struct  _TimeoutDialog TimeoutDialog;

class XrandrManager : QObject
{
    Q_OBJECT
    Q_CLASSINFO ("D-Bus Interface", USD_XRANDR_DBUS_NAME)
private:
    XrandrManager();
    XrandrManager(XrandrManager&)=delete;
    XrandrManager&operator=(const XrandrManager&)=delete;

public:
    ~XrandrManager();
    static XrandrManager* XrandrManagerNew();
    bool XrandrManagerStart(GError** error);
    void XrandrManagerStop();

    static void log_open();
    static void log_close();
    static void log_msg(const char* format,...);
    static void log_output(MateRROutputInfo *output);
    static void log_screen(MateRRScreen *screen);
    static char timestamp_relationship (unsigned int a,unsigned int b);
    static void log_configuration (MateRRConfig *config);

    static void on_randr_event(MateRRScreen *screen);
    static void show_timestamps_dialog(const char *msg);

    static gboolean apply_configuration_from_filename (XrandrManager    *manager,
                                                       const char       *filename,
                                                       gboolean          no_matching_config_is_an_error,
                                                       unsigned int      timestamp,
                                                       GError           **error);
    static void auto_configure_outputs (unsigned int timestamp);
    static gboolean apply_configuration_and_display_error (XrandrManager *manager,
                                                           MateRRConfig *config,
                                                           unsigned int timestamp);

    static void error_message (XrandrManager    *manager,
                               const char *primary_text,
                               GError *error_to_display,
                               const char *secondary_text);
    static void apply_color_profiles (void);
    static void refresh_tray_icon_menu_if_active(unsigned int timestamp);
    static void status_icon_popup_menu (XrandrManager *manager,
                                        unsigned int button,
                                        unsigned int timestamp);
    static void add_menu_items_for_outputs (void);
    static void add_menu_items_for_output(MateRROutputInfo *output);

    static void popup_menu_configure_display_cb (GtkMenuItem *item,gpointer data);
    static void run_display_capplet (GtkWidget *widget);
    static void status_icon_popup_menu_selection_done_cb (GtkMenuShell *menu_shell,gpointer data);
    static GtkWidget * make_menu_item_for_output_title ( MateRROutputInfo *output);

    static void title_item_size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation);

    static gboolean output_title_label_draw_cb (GtkWidget *widget, cairo_t *cr);

    static gboolean output_title_label_after_draw_cb (GtkWidget *widget, cairo_t *cr);

    static void add_rotation_items_for_output (MateRROutputInfo *output);

    static void get_allowed_rotations_for_output (MateRRConfig *config,
                                                  MateRRScreen *rr_screen,
                                                  MateRROutputInfo *output,
                                                  int *out_num_rotations,
                                                  MateRRRotation *out_rotations);

    static void add_unsupported_rotation_item (void);
    static void add_items_for_rotations (MateRROutputInfo *output,
                                         MateRRRotation allowed_rotations);

    static void output_rotation_item_activate_cb (GtkCheckMenuItem *item);

    static void ensure_current_configuration_is_saved (void);
    static gboolean try_to_apply_intended_configuration (GdkWindow *parent_window,
                                                         guint32 timestamp,
                                                         GError **error);

    static void restore_backup_configuration_without_messages (const char *backup_filename,
                                                           const char *intended_filename);

    static void queue_confirmation_by_user (GdkWindow *parent_window, guint32 timestamp);
    static gboolean confirm_with_user_idle_cb (gpointer data);
    static gboolean user_says_things_are_ok (XrandrManager *managers, GdkWindow *parent_window);
    static void print_countdown_text (TimeoutDialog *timeout);
    static void timeout_response_cb (GtkDialog *dialog, int response_id, gpointer data);
    static gboolean timeout_cb (gpointer data);
    static void restore_backup_configuration (XrandrManager *manager,
                                              const char *backup_filename,
                                              const char *intended_filename,
                                              unsigned int timestamp);

    static void on_config_changed (GSettings        *settings,
                                    gchar            *key,
                                    XrandrManager *manager);

    static void start_or_stop_icon (XrandrManager *manager);
    static void status_icon_start (XrandrManager *manager);
    static void status_icon_stop (XrandrManager *manager);
    static void status_icon_activate_cb (GtkStatusIcon *status_icon,
                                         gpointer data);

    static void status_icon_popup_menu_cb (GtkStatusIcon *status_icon,
                                           unsigned int button,
                                           unsigned int timestamp,
                                           gpointer data);

    static gboolean apply_stored_configuration_at_startup (XrandrManager *manager, guint32 timestamp);
    static gboolean apply_default_configuration_from_file (XrandrManager *manager, guint32 timestamp);
    static void apply_default_boot_configuration (XrandrManager *mgr, guint32 timestamp);
    static GdkFilterReturn event_filter (GdkXEvent           *xevent,
                                         GdkEvent            *event,
                                         gpointer             data);

    static gboolean apply_intended_configuration (XrandrManager *manager,
                                                  const char *intended_filename,
                                                  guint32 timestamp);

    static MateRRConfig * make_clone_setup(MateRRScreen *screen);
    static gboolean config_is_all_off (MateRRConfig *config);
    static void print_configuration (MateRRConfig *config, const char *header);
    static void print_output (MateRROutputInfo *info);
    static gboolean get_clone_size (MateRRScreen *screen, int *width, int *height);
    static MateRRConfig * make_laptop_setup (MateRRScreen *screen);
    static gboolean is_laptop (MateRRScreen *screen, MateRROutputInfo *output);
    static gboolean turn_on (MateRRScreen *screen,
                            MateRROutputInfo *info,
                            int x, int y);

    static MateRRMode *find_best_mode (MateRROutput *output);
    static MateRRConfig * make_other_setup (MateRRScreen *screen);
    static void handle_fn_f7 (XrandrManager *mgr, guint32 timestamp);
    static void generate_fn_f7_configs (XrandrManager *mgr);
    static MateRRConfig * make_xinerama_setup (MateRRScreen *screen);
    static int turn_on_and_get_rightmost_offset (MateRRScreen *screen, MateRROutputInfo *info, int x);
    static GPtrArray * sanitize (XrandrManager *manager, GPtrArray *array);
    static void log_configurations (MateRRConfig **configs);
    static void handle_rotate_windows (XrandrManager *mgr, guint32 timestamp);
    static MateRROutputInfo * get_laptop_output_info (MateRRScreen *screen, MateRRConfig *config);
    static MateRRRotation get_next_rotation (MateRRRotation allowed_rotations, MateRRRotation current_rotation);


private:
    static XrandrManager* mXrandrManager;

    /* Key code of the XF86Display key (Fn-F7 on Thinkpads, Fn-F4 on HP machines, etc.) */
    guint switch_video_mode_keycode;

    /* Key code of the XF86RotateWindows key (present on some tablets) */
    guint rotate_windows_keycode;

    MateRRScreen *rw_screen;
    gboolean running;

    GtkStatusIcon *status_icon;     
    GtkWidget *popup_menu;          

    MateRRConfig *configuration;
    MateRRLabeler *labeler;
    GSettings *settings;

    /* fn-F7 status */
    int     current_fn_f7_config;  /* -1 if no configs */
    MateRRConfig **fn_f7_configs;  /* NULL terminated, NULL if there are no configs */
    
     /* Last time at which we got a "screen got reconfigured" event; see on_randr_event() */
     guint32 last_config_timestamp;
};

#endif // XRANDRMANAGER_H
