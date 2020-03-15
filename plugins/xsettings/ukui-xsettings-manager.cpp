#include "ukui-xsettings-manager.h"
#include "xsettings-manager.h"
#include "fontconfig-monitor.h"
#include "ukui-xft-settings.h"
#include "xsettings-const.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <syslog.h>

//#define UKUI_XSETTINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UKUI_TYPE_XSETTINGS_MANAGER, ukuiXSettingsManagerPrivate))

#define MOUSE_SCHEMA          "org.ukui.peripherals-mouse"
#define INTERFACE_SCHEMA      "org.mate.interface"
#define SOUND_SCHEMA          "org.mate.sound"
#define CURSOR_THEME_KEY      "cursor-theme"
#define CURSOR_SIZE_KEY       "cursor-size"

/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_gsettings_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

typedef struct _TranslationEntry TranslationEntry;
typedef void (* TranslationFunc) (ukuiXSettingsManager  *manager,
                                  TranslationEntry      *trans,
                                  GVariant              *value);

struct _TranslationEntry {
    const char     *gsettings_schema;
    const char     *gsettings_key;
    const char     *xsetting_name;

    TranslationFunc translate;
};

struct ukuiXSettingsManagerPrivate
{
    XsettingsManager **pManagers;
    GHashTable *gsettings;
    GSettings *gsettings_font;
    // fontconfig_monitor_handle_t *fontconfig_handle;
};

#define USD_XSETTINGS_ERROR usd_xsettings_error_quark ()

enum {
    USD_XSETTINGS_ERROR_INIT
};

static void     ukui_xsettings_manager_finalize    (GObject                  *object);
static void     fontconfig_callback (fontconfig_monitor_handle_t *handle,
                                     ukuiXSettingsManager       *manager);
static void
xft_callback (GSettings            *gsettings,
              const gchar          *key,
              ukuiXSettingsManager *manager);

// G_DEFINE_TYPE (ukuiXSettingsManager, ukui_xsettings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static GQuark
usd_xsettings_error_quark (void)
{
    return g_quark_from_static_string ("usd-xsettings-error-quark");
}

static void
translate_bool_int (ukuiXSettingsManager  *manager,
                    TranslationEntry      *trans,
                    GVariant              *value)
{
    int i;

    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->set_int (trans->xsetting_name,
                                         g_variant_get_boolean (value));
    }
}

static void
translate_int_int (ukuiXSettingsManager  *manager,
                   TranslationEntry      *trans,
                   GVariant              *value)
{
    int i;

    for (i = 0; manager-> pManagers [i]; i++) {
        manager-> pManagers [i]->set_int (trans->xsetting_name,
                                          g_variant_get_int32 (value));
    }
}

static void
translate_string_string (ukuiXSettingsManager  *manager,
                         TranslationEntry      *trans,
                         GVariant              *value)
{
    int i;

    for (i = 0; manager-> pManagers [i]; i++) {
        manager-> pManagers [i]->set_string (trans->xsetting_name,
                                             g_variant_get_string (value, NULL));
    }
}
static void
translate_string_string_toolbar (ukuiXSettingsManager  *manager,
                                 TranslationEntry      *trans,
                                 GVariant              *value)
{
    int         i;
    const char *tmp;

    /* This is kind of a workaround since GNOME expects the key value to be
         * "both_horiz" and gtk+ wants the XSetting to be "both-horiz".
         */
    tmp = g_variant_get_string (value, NULL);
    if (tmp && strcmp (tmp, "both_horiz") == 0) {
        tmp = "both-horiz";
    }

    for (i = 0; manager-> pManagers [i]; i++) {
        manager-> pManagers [i]->set_string (trans->xsetting_name,
                                             tmp);
    }
}

static TranslationEntry translations [] = {
    { MOUSE_SCHEMA,     "double-click",           "Net/DoubleClickTime",           translate_int_int },
    { MOUSE_SCHEMA,     "drag-threshold",         "Net/DndDragThreshold",          translate_int_int },
    { MOUSE_SCHEMA,     "cursor-theme",           "Gtk/CursorThemeName",           translate_string_string },
    { MOUSE_SCHEMA,     "cursor-size",            "Gtk/CursorThemeSize",           translate_int_int },

    { INTERFACE_SCHEMA, "font-name",              "Gtk/FontName",                  translate_string_string },
    { INTERFACE_SCHEMA, "gtk-key-theme",          "Gtk/KeyThemeName",              translate_string_string },
    { INTERFACE_SCHEMA, "toolbar-style",          "Gtk/ToolbarStyle",              translate_string_string_toolbar },
    { INTERFACE_SCHEMA, "toolbar-icons-size",     "Gtk/ToolbarIconSize",           translate_string_string },
    { INTERFACE_SCHEMA, "cursor-blink",           "Net/CursorBlink",               translate_bool_int },
    { INTERFACE_SCHEMA, "cursor-blink-time",      "Net/CursorBlinkTime",           translate_int_int },
    { INTERFACE_SCHEMA, "gtk-theme",              "Net/ThemeName",                 translate_string_string },
    { INTERFACE_SCHEMA, "gtk-color-scheme",       "Gtk/ColorScheme",               translate_string_string },
    { INTERFACE_SCHEMA, "gtk-im-preedit-style",   "Gtk/IMPreeditStyle",            translate_string_string },
    { INTERFACE_SCHEMA, "gtk-im-status-style",    "Gtk/IMStatusStyle",             translate_string_string },
    { INTERFACE_SCHEMA, "gtk-im-module",          "Gtk/IMModule",                  translate_string_string },
    { INTERFACE_SCHEMA, "icon-theme",             "Net/IconThemeName",             translate_string_string },
    { INTERFACE_SCHEMA, "file-chooser-backend",   "Gtk/FileChooserBackend",        translate_string_string },
    { INTERFACE_SCHEMA, "gtk-decoration-layout",  "Gtk/DecorationLayout",          translate_string_string },
    { INTERFACE_SCHEMA, "gtk-shell-shows-app-menu","Gtk/ShellShowsAppMenu",        translate_bool_int },
    { INTERFACE_SCHEMA, "gtk-shell-shows-menubar","Gtk/ShellShowsMenubar",         translate_bool_int },
    { INTERFACE_SCHEMA, "menus-have-icons",       "Gtk/MenuImages",                translate_bool_int },
    { INTERFACE_SCHEMA, "buttons-have-icons",     "Gtk/ButtonImages",              translate_bool_int },
    { INTERFACE_SCHEMA, "menubar-accel",          "Gtk/MenuBarAccel",              translate_string_string },
    { INTERFACE_SCHEMA, "show-input-method-menu", "Gtk/ShowInputMethodMenu",       translate_bool_int },
    { INTERFACE_SCHEMA, "show-unicode-menu",      "Gtk/ShowUnicodeMenu",           translate_bool_int },
    { INTERFACE_SCHEMA, "automatic-mnemonics",    "Gtk/AutoMnemonics",             translate_bool_int },
    { INTERFACE_SCHEMA, "gtk-enable-animations",  "Gtk/EnableAnimations",          translate_bool_int },
    { INTERFACE_SCHEMA, "gtk-dialogs-use-header", "Gtk/DialogsUseHeader",          translate_bool_int },

    { SOUND_SCHEMA, "theme-name",                 "Net/SoundThemeName",            translate_string_string },
    { SOUND_SCHEMA, "event-sounds",               "Net/EnableEventSounds" ,        translate_bool_int },
    { SOUND_SCHEMA, "input-feedback-sounds",      "Net/EnableInputFeedbackSounds", translate_bool_int }
};

static gboolean
start_fontconfig_monitor_idle_cb (ukuiXSettingsManager *manager)
{
    // // ukui_settings_profile_start (NULL);

    manager-> fontconfig_handle = fontconfig_monitor_start ((GFunc) fontconfig_callback, manager);

    // ukui_settings_profile_end (NULL);

    return FALSE;
}

static void
start_fontconfig_monitor (ukuiXSettingsManager  *manager)
{
    //// ukui_settings_profile_start (NULL);

    fontconfig_cache_init ();

    g_idle_add ((GSourceFunc) start_fontconfig_monitor_idle_cb, manager);

    //// ukui_settings_profile_end (NULL);
}

static void
stop_fontconfig_monitor (ukuiXSettingsManager  *manager)
{
    if (manager->fontconfig_handle) {
        fontconfig_monitor_stop (manager->fontconfig_handle);
        manager->fontconfig_handle = NULL;
    }
}

static void
process_value (ukuiXSettingsManager *manager,
               TranslationEntry     *trans,
               GVariant             *value)
{
    (* trans->translate) (manager, trans, value);
}

static void
terminate_cb (void *data)
{
    gboolean *terminated = reinterpret_cast<gboolean*>(data);

    if (*terminated) {
        return;
    }

    *terminated = TRUE;

    gtk_main_quit ();
}



ukuiXSettingsManager::ukuiXSettingsManager()
{
    GdkDisplay *display;
    int         i;
    int         n_screens;
    gboolean    res;
    gboolean    terminated;

    display = gdk_display_get_default ();
    n_screens = gdk_display_get_n_screens (display);

    res = xsettings_manager_check_running (gdk_x11_display_get_xdisplay (display),
                                           gdk_screen_get_number (gdk_screen_get_default ()));
    if (res) {
        g_warning ("You can only run one xsettings manager at a time; exiting");
        // throw error!
    }

    pManagers = new XsettingsManager*[n_screens + 1];
    for (i = 0; i < n_screens; i++) {
        GdkScreen *screen;

        screen = gdk_display_get_screen (display, i);

        pManagers[i] = new XsettingsManager (gdk_x11_display_get_xdisplay (display),
                                             gdk_screen_get_number (screen),
                                             terminate_cb,
                                             &terminated);

    }


}

ukuiXSettingsManager::~ukuiXSettingsManager()
{

}

static TranslationEntry *
find_translation_entry (GSettings *gsettings, const char *key)
{
    guint i;
    char *schema;

    g_object_get (gsettings, "schema", &schema, NULL);

    for (i = 0; i < G_N_ELEMENTS (translations); i++) {
        if (g_str_equal (schema, translations[i].gsettings_schema) &&
                g_str_equal (key, translations[i].gsettings_key)) {
            g_free (schema);
            return &translations[i];
        }
    }

    g_free (schema);

    return NULL;
}


static void
xsettings_callback (GSettings             *gsettings,
                    const char            *key,
                    ukuiXSettingsManager  *manager)
{
    TranslationEntry *trans;
    int               i;
    GVariant         *value;

    if (g_str_equal (key, CURSOR_THEME_KEY) ||
            g_str_equal (key, CURSOR_SIZE_KEY)) {
        xft_callback (NULL, key, manager);
        return;
    }

    trans = find_translation_entry (gsettings, key);
    if (trans == NULL) {
        return;
    }

    value = g_settings_get_value (gsettings, key);

    process_value (manager, trans, value);

    g_variant_unref (value);

    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->set_string ("Net/FallbackIconTheme",
                                            "ukui");
    }

    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->notify ();
    }
}


static gboolean
setup_xsettings_managers (ukuiXSettingsManager *manager)
{
    GdkDisplay *display;
    int         i;
    int         n_screens;
    gboolean    res;
    gboolean    terminated;

    display = gdk_display_get_default ();
    n_screens = gdk_display_get_n_screens (display);

    res = xsettings_manager_check_running (gdk_x11_display_get_xdisplay (display),
                                           gdk_screen_get_number (gdk_screen_get_default ()));
    if (res) {
        g_warning ("You can only run one xsettings manager at a time; exiting");
        return FALSE;
    }

    terminated = FALSE;
    for (i = 0; i < n_screens; i++) {
        GdkScreen *screen;

        screen = gdk_display_get_screen (display, i);

        manager->pManagers [i] = new XsettingsManager (gdk_x11_display_get_xdisplay (display),
                                                       gdk_screen_get_number (screen),
                                                       terminate_cb,
                                                       &terminated);
        if (! manager->pManagers [i]) {
            g_warning ("Could not create xsettings manager for screen %d!", i);
            return FALSE;
        }
    }

    return TRUE;
}
void update_xft_settings (ukuiXSettingsManager *manager)
{
    UkuiXftSettings settings;

    //ukui_settings_profile_start (NULL);

    settings.xft_settings_get (manager);
    settings.xft_settings_set_xsettings (manager);
    settings.xft_settings_set_xresources ();

    //ukui_settings_profile_end (NULL);
}

static void
xft_callback (GSettings            *gsettings,
              const gchar          *key,
              ukuiXSettingsManager *manager)
{
    int i;
    //update_xft_settings (manager);
    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->notify ();
    }
}

int ukuiXSettingsManager::start(GError               **error)
{
    guint        i;
    GList       *list, *l;

    syslog (LOG_ERR, "Starting xsettings manager");
    // // ukui_settings_profile_start (NULL);

    if (!setup_xsettings_managers (this)) {
        g_set_error (error, USD_XSETTINGS_ERROR,
                     USD_XSETTINGS_ERROR_INIT,
                     "Could not initialize xsettings manager.");
        return FALSE;
    }

    gsettings = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       NULL, (GDestroyNotify) g_object_unref);

    g_hash_table_insert ( gsettings,
                          (void*)MOUSE_SCHEMA, g_settings_new (MOUSE_SCHEMA));
    g_hash_table_insert ( gsettings,
                          (void*)INTERFACE_SCHEMA, g_settings_new (INTERFACE_SCHEMA));
    g_hash_table_insert ( gsettings,
                          (void*)SOUND_SCHEMA, g_settings_new (SOUND_SCHEMA));

    list = g_hash_table_get_values ( gsettings);
    for (l = list; l != NULL; l = l->next) {
        g_signal_connect_object (G_OBJECT (l->data), "changed",
                                 G_CALLBACK (xsettings_callback), this, (GConnectFlags)0);
    }
    g_list_free (list);
    for (i = 0; i < G_N_ELEMENTS (translations); i++) {
        GVariant  *val;
        GSettings *gsettings;

        gsettings = (GSettings *)g_hash_table_lookup ( this->gsettings,
                                                       translations[i].gsettings_schema);

        if (gsettings == NULL) {
            g_warning ("Schemas '%s' has not been setup", translations[i].gsettings_schema);
            continue;
        }

        val = g_settings_get_value (gsettings, translations[i].gsettings_key);

        process_value (this, &translations[i], val);
        g_variant_unref (val);
    }

    gsettings_font = g_settings_new (FONT_RENDER_SCHEMA);
    g_signal_connect ( gsettings_font, "changed", G_CALLBACK (xft_callback), pManagers);
    update_xft_settings (this);

    start_fontconfig_monitor (this);

    for (i = 0;  pManagers [i]; i++)
        pManagers [i]->set_string ( "Net/FallbackIconTheme",
                                    "ukui");

    for (i = 0;  pManagers [i]; i++) {
        pManagers [i]->notify ( );
    }

    //// ukui_settings_profile_end (NULL);

    return TRUE;
}

static void
fontconfig_callback (fontconfig_monitor_handle_t *handle,
                     ukuiXSettingsManager       *manager)
{
    int i;
    int timestamp = time (NULL);

    // ukui_settings_profile_start (NULL);

    for (i = 0; manager-> pManagers [i]; i++) {
        manager-> pManagers [i]->set_int ("Fontconfig/Timestamp", timestamp);
        manager-> pManagers [i]->notify ();
    }
    // ukui_settings_profile_end (NULL);
}

int ukuiXSettingsManager::stop()
{
    int i;
    if (pManagers != NULL) {
        for (i = 0; pManagers [i]; ++i) {
            delete (pManagers[i]);
            pManagers[i] = NULL;
        }
    }

    if (gsettings != NULL) {
        g_hash_table_destroy (gsettings);
        gsettings = NULL;
    }

    if (gsettings_font != NULL) {
        g_object_unref (gsettings_font);
        gsettings_font = NULL;
    }

    stop_fontconfig_monitor (this);

}
