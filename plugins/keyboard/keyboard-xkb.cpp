#include <QIcon>
#include "keyboard-xkb.h"
#include "clib-syslog.h"

#define MATEKBD_DESKTOP_SCHEMA  "org.mate.peripherals-keyboard-xkb.general"
#define MATEKBD_KBD_SCHEMA      "org.mate.peripherals-keyboard-xkb.kbd"
#define KNOWN_FILES_KEY         "known-file-list"
#define DISABLE_INDICATOR_KEY   "disable-indicator"

KeyboardManager  *KeyboardXkb::manager  = KeyboardManager::KeyboardManagerNew();
static void      *pa_callback_user_data = NULL;

static XklConfigRegistry*  xkl_registry = NULL;
static MatekbdDesktopConfig  current_desktop_config;
static MatekbdKeyboardConfig current_kbd_config;

/* never terminated */
static MatekbdKeyboardConfig  initial_sys_kbd_config;
static PostActivationCallback pa_callback  = NULL;
static XklEngine*             xkl_engine;
static bool                   inited_ok = false;

KeyboardXkb::KeyboardXkb()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard Xkb initializing!");
}

KeyboardXkb::~KeyboardXkb()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard Xkb free");
    if(settings_desktop)
        delete settings_desktop;
    if(settings_kbd)
        delete settings_kbd;
}

void KeyboardXkb::usd_keyboard_xkb_analyze_sysconfig (void)
{
    if (!inited_ok)
        return;
    matekbd_keyboard_config_init (&initial_sys_kbd_config, xkl_engine);
    matekbd_keyboard_config_load_from_x_initial (&initial_sys_kbd_config,NULL);
}

void KeyboardXkb::apply_desktop_settings (void)
{
    if (!inited_ok)
        return;

    manager->usd_keyboard_manager_apply_settings (manager);
    matekbd_desktop_config_load_from_gsettings (&current_desktop_config);

    /* again, probably it would be nice to compare things
       before activating them */
    matekbd_desktop_config_activate (&current_desktop_config);
}

bool KeyboardXkb::try_activating_xkb_config_if_new (MatekbdKeyboardConfig *current_sys_kbd_config)
{
    /* Activate - only if different! */
    if (!matekbd_keyboard_config_equals
        (&current_kbd_config, current_sys_kbd_config)) {
        if (matekbd_keyboard_config_activate (&current_kbd_config)) {
            if (pa_callback != NULL) {
                (*pa_callback) (pa_callback_user_data);
                return TRUE;
            }
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

static void g_strv_behead (gchar **arr)
{
    if (arr == NULL || *arr == NULL)
        return;

    g_free (*arr);
    memmove (arr, arr + 1, g_strv_length (arr) * sizeof (gchar *));
}


bool KeyboardXkb::filter_xkb_config (void)
{
    XklConfigItem *item;
    char *lname;
    char *vname;
    char **lv;
    bool any_change = FALSE;

    xkl_debug (100, "Filtering configuration against the registry\n");

    if (!xkl_registry) {
        xkl_registry = xkl_config_registry_get_instance (xkl_engine);

        /* load all materials, unconditionally! */
        if (!xkl_config_registry_load (xkl_registry, TRUE)){
            g_object_unref (xkl_registry);
            xkl_registry = NULL;
            return FALSE;
        }
    }
    lv = current_kbd_config.layouts_variants;
    item = xkl_config_item_new ();
    while (*lv) {
        xkl_debug (100, "Checking [%s]\n", *lv);

        if (matekbd_keyboard_config_split_items (*lv, &lname, &vname)) {

            bool should_be_dropped = FALSE;
            snprintf (item->name ,sizeof (item->name), "%s",lname);
            if (!xkl_config_registry_find_layout(xkl_registry, item)) {

                xkl_debug (100, "Bad layout [%s]\n",lname);
                should_be_dropped = TRUE;
            } else if (vname) {
                snprintf (item->name,sizeof (item->name), "%s", vname);
                if (!xkl_config_registry_find_variant
                    (xkl_registry, lname, item)) {
                    xkl_debug (100,"Bad variant [%s(%s)]\n",lname, vname);
                    should_be_dropped = TRUE;
                }
            }
            if (should_be_dropped) {
                g_strv_behead (lv);
                any_change = TRUE;
                continue;
            }
        }
        lv++;
    }
    g_object_unref (item);
    return any_change;
}

static void activation_error (void)
{
    Display *dpy = QX11Info::display();
    char const *vendor = ServerVendor (dpy);//GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
    int release = VendorRelease (dpy);//GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
    GtkWidget *dialog;

    /* VNC viewers will not work, do not barrage them with warnings */
    if (NULL != vendor && NULL != strstr (vendor, "VNC"))
        return;

    dialog = gtk_message_dialog_new_with_markup (NULL,
                             (GtkDialogFlags)0,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             _("Error activating XKB configuration.\n"
                              "It can happen under various circumstances:\n"
                              " • a bug in libxklavier library\n"
                              " • a bug in X server (xkbcomp, xmodmap utilities)\n"
                              " • X server with incompatible libxkbfile implementation\n\n"
                              "X server version data:\n%s\n%d\n"
                              "If you report this situation as a bug, please include:\n"
                              " • The result of <b>%s</b>\n"
                              " • The result of <b>%s</b>"),
                             vendor,
                             release,
                             "xprop -root | grep XKB",
                             "gsettings list-keys org.mate.peripherals-keyboard-xkb.kbd");
    g_signal_connect (dialog, "response",
              G_CALLBACK (gtk_widget_destroy), NULL);
    //usd_delayed_show_dialog (dialog);//缺少函数,后期填加
}


void KeyboardXkb::apply_xkb_settings (void)
{
    MatekbdKeyboardConfig current_sys_kbd_config;

    if (!inited_ok)
        return;

    matekbd_keyboard_config_init (&current_sys_kbd_config, xkl_engine);

    matekbd_keyboard_config_load_from_gsettings (&current_kbd_config,
                          &initial_sys_kbd_config);

    matekbd_keyboard_config_load_from_x_current (&current_sys_kbd_config,
                          NULL);

    if (!try_activating_xkb_config_if_new (&current_sys_kbd_config)) {
        if (filter_xkb_config ()) {
            if (!try_activating_xkb_config_if_new(&current_sys_kbd_config)) {
                qWarning ("Could not activate the filtered XKB configuration");
                //activation_error (); // 缺少弹窗,后期填加
            }
        } else {
            qWarning("Could not activate the XKB configuration");
            //activation_error ();
        }
    } else
        xkl_debug (100, "Actual KBD configuration was not changed: redundant notification\n");

    matekbd_keyboard_config_term (&current_sys_kbd_config);
}

void KeyboardXkb::apply_desktop_settings_cb (QString key)
{
    apply_desktop_settings ();
}
void KeyboardXkb::apply_desktop_settings_mate_cb(GSettings *settings,char *key)
{
    apply_desktop_settings ();
}

void KeyboardXkb::apply_xkb_settings_cb (QString key)
{
    apply_xkb_settings ();
}
void KeyboardXkb::apply_xkb_settings_mate_cb (GSettings *settings,char *key)
{
    apply_xkb_settings ();
}

GdkFilterReturn usd_keyboard_xkb_evt_filter (GdkXEvent * xev, GdkEvent * event,gpointer data)
{
    KeyboardXkb *xkb = (KeyboardXkb *)data;
    XEvent *xevent = (XEvent *) xev;

    xkl_engine_filter_events (xkl_engine, xevent);
    return GDK_FILTER_CONTINUE;
}

/* When new Keyboard is plugged in - reload the settings */
void KeyboardXkb::usd_keyboard_new_device (XklEngine * engine)
{
    apply_desktop_settings ();
    apply_xkb_settings ();
}


void KeyboardXkb::usd_keyboard_xkb_init(KeyboardManager* kbd_manager)
{
    CT_SYSLOG(LOG_DEBUG,"init --- XKB");
    Display *display;

    display    = QX11Info::display();
    manager    = kbd_manager;
    xkl_engine = xkl_engine_get_instance (display);

    if (xkl_engine) {
        inited_ok = TRUE;
        settings_desktop = new QGSettings(MATEKBD_DESKTOP_SCHEMA);
        settings_kbd     = new QGSettings(MATEKBD_KBD_SCHEMA);

        matekbd_desktop_config_init (&current_desktop_config,xkl_engine);
        matekbd_keyboard_config_init (&current_kbd_config,xkl_engine);
        xkl_engine_backup_names_prop (xkl_engine);

        usd_keyboard_xkb_analyze_sysconfig ();

        matekbd_desktop_config_start_listen (&current_desktop_config, G_CALLBACK (apply_desktop_settings_mate_cb),NULL);

        matekbd_keyboard_config_start_listen (&current_kbd_config,G_CALLBACK (apply_xkb_settings_mate_cb),NULL);

        QObject::connect(settings_desktop,SIGNAL(changed(QString)),this,SLOT(apply_desktop_settings_cb(QString)));

        QObject::connect(settings_kbd,SIGNAL(changed(QString)),this,SLOT(apply_xkb_settings_cb(QString)));

        gdk_window_add_filter (NULL, (GdkFilterFunc)usd_keyboard_xkb_evt_filter, this);

        if (xkl_engine_get_features (xkl_engine) &XKLF_DEVICE_DISCOVERY)
            g_signal_connect (xkl_engine, "X-new-device",
                              G_CALLBACK(usd_keyboard_new_device), NULL);

        xkl_engine_start_listen (xkl_engine, XKLL_MANAGE_LAYOUTS |XKLL_MANAGE_WINDOW_STATES);

        apply_desktop_settings ();
        apply_xkb_settings ();
    }
}

void KeyboardXkb::usd_keyboard_xkb_shutdown (void)
{
    pa_callback = NULL;
    pa_callback_user_data = NULL;
    manager = NULL;

    if (!inited_ok)
        return;

    xkl_engine_stop_listen (xkl_engine,
                            XKLL_MANAGE_LAYOUTS |
                            XKLL_MANAGE_WINDOW_STATES);

    gdk_window_remove_filter (NULL, (GdkFilterFunc)usd_keyboard_xkb_evt_filter, NULL);

    if (xkl_registry) {
        g_object_unref (xkl_registry);
    }
    g_object_unref (xkl_engine);

    xkl_engine = NULL;
    inited_ok = FALSE;
}
