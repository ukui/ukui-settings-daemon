/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QGuiApplication>
#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QScreen>

#include "ukui-xsettings-manager.h"
#include "xsettings-manager.h"
#include "fontconfig-monitor.h"
#include "ukui-xft-settings.h"
#include "xsettings-const.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <syslog.h>
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

static void     fontconfig_callback (fontconfig_monitor_handle_t *handle,
                                     ukuiXSettingsManager       *manager);
static void
xft_callback (GSettings            *gsettings,
              const gchar          *key,
              ukuiXSettingsManager *manager);

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

static gboolean start_fontconfig_monitor_idle_cb (ukuiXSettingsManager *manager)
{
    manager-> fontconfig_handle = fontconfig_monitor_start ((GFunc) fontconfig_callback, manager);

    return FALSE;
}

static void start_fontconfig_monitor (ukuiXSettingsManager  *manager)
{

    fontconfig_cache_init ();

    g_idle_add ((GSourceFunc) start_fontconfig_monitor_idle_cb, manager);

}

static void stop_fontconfig_monitor (ukuiXSettingsManager  *manager)
{
    if (manager->fontconfig_handle) {
        fontconfig_monitor_stop (manager->fontconfig_handle);
        manager->fontconfig_handle = NULL;
    }
}

static void process_value (ukuiXSettingsManager *manager,
               TranslationEntry     *trans,
               GVariant             *value)
{
    (* trans->translate) (manager, trans, value);
}

static void terminate_cb (void *data)
{
    gboolean *terminated = reinterpret_cast<gboolean*>(data);
    if (*terminated) {
        return;
    }
    *terminated = TRUE;
    exit(0);//gtk_main_quit ();
}

ukuiXSettingsManager::ukuiXSettingsManager()
{
    gdk_init(NULL,NULL);
    pManagers=nullptr;
    gsettings=nullptr;
    gsettings_font=nullptr;
    fontconfig_handle=nullptr;
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
    syslog(LOG_ERR,"%s  key=%s",__func__,key);
    if (g_str_equal (key, CURSOR_THEME_KEY)||
        g_str_equal (key, CURSOR_SIZE_KEY )||
        g_str_equal (key,SCALING_FACTOR_KEY)){
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
    gboolean    res;
    gboolean    terminated;

    display = gdk_display_get_default ();
    res = xsettings_manager_check_running (gdk_x11_display_get_xdisplay (display),0);

    if (res) {
        g_warning ("You can only run one xsettings manager at a time; exiting");
        return FALSE;
    }

    if(!manager->pManagers)
        manager->pManagers = new XsettingsManager*[2];

    for(int i=0;manager->pManagers[i];i++)
        manager->pManagers [i] = nullptr;

    terminated = FALSE;
    if(!manager->pManagers [0])
        manager->pManagers [0] = new XsettingsManager (gdk_x11_display_get_xdisplay (display),
                                                       0,
                                                       (XSettingsTerminateFunc)terminate_cb,
                                                       &terminated);
    if (! manager->pManagers [0]) {
        g_warning ("Could not create xsettings manager for screen!");
        return FALSE;
    }

    return TRUE;
}

void update_xft_settings (ukuiXSettingsManager *manager)
{
    UkuiXftSettings settings;

    settings.xft_settings_get (manager);
    settings.xft_settings_set_xsettings (manager);
    settings.xft_settings_set_xresources ();
}

static void
xft_callback (GSettings            *gsettings,
              const gchar          *key,
              ukuiXSettingsManager *manager)
{
    int i;

    update_xft_settings (manager);
    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->notify ();
    }
}



static void
fontconfig_callback (fontconfig_monitor_handle_t *handle,
                     ukuiXSettingsManager       *manager)
{
    int i;
    int timestamp = time (NULL);

    for (i = 0; manager-> pManagers [i]; i++) {
        manager-> pManagers [i]->set_int ("Fontconfig/Timestamp", timestamp);
        manager-> pManagers [i]->notify ();
    }
}

void ukuiXSettingsManager::sendSessionDbus()
{
    QDBusMessage message = QDBusMessage::createMethodCall("org.gnome.SessionManager",
                                                          "/org/gnome/SessionManager",
                                                          "org.gnome.SessionManager",
                                                          "startupfinished");
    QList<QVariant> args;
    args.append("ukui-settings-daemon");
    args.append("startupfinished");
    message.setArguments(args);
    QDBusConnection::sessionBus().send(message);
}

bool ukuiXSettingsManager::start()
{
    guint        i;
    GList       *list, *l;
    syslog(LOG_ERR,"Xsettings manager start");

    if (!setup_xsettings_managers (this)) {
        qDebug ("Could not initialize xsettings manager.");
        USD_XSETTINGS_ERROR;
        return false;
    }
    gsettings = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       NULL, (GDestroyNotify) g_object_unref);
    g_hash_table_insert ( gsettings,(void*)MOUSE_SCHEMA,
                          g_settings_new (MOUSE_SCHEMA));
    g_hash_table_insert ( gsettings,(void*)INTERFACE_SCHEMA,
                          g_settings_new (INTERFACE_SCHEMA));
    g_hash_table_insert ( gsettings,(void*)SOUND_SCHEMA,
                          g_settings_new (SOUND_SCHEMA));
    g_hash_table_insert (gsettings,(void*)XSETTINGS_PLUGIN_SCHEMA,
                         g_settings_new (XSETTINGS_PLUGIN_SCHEMA));

    list = g_hash_table_get_values ( gsettings);
    for (l = list; l != NULL; l = l->next) {
        g_signal_connect (G_OBJECT (l->data), "changed",
                                 G_CALLBACK (xsettings_callback), this);
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

    this->gsettings_font = g_settings_new (FONT_RENDER_SCHEMA);
    g_signal_connect (this->gsettings_font, "changed", G_CALLBACK (xft_callback), this);

    GSettings   *gsettings;
    double       scale;
    gsettings = (GSettings *)g_hash_table_lookup(this->gsettings, XSETTINGS_PLUGIN_SCHEMA);
    scale = g_settings_get_double (gsettings, SCALING_FACTOR_KEY);
    if(scale > 1.0){
        bool state = false;
        int screenNum = QGuiApplication::screens().length();
        for(int i = 0; i < screenNum; i++){
            QScreen *screen = QGuiApplication::screens().at(i);
            //qDebug()<<screen->geometry();
            if (screen->geometry().width() <= 1920 &&  screen->geometry().height() <= 1080)
                state = true;
            else {
                state = false;
                break;
            }
        }
        if (state){
            GSettings   *mGsettings;
            mGsettings = (GSettings *)g_hash_table_lookup(this->gsettings, MOUSE_SCHEMA);
            g_settings_set_double (mGsettings, CURSOR_SIZE_KEY, 24);
            g_settings_set_double (gsettings, SCALING_FACTOR_KEY, 1.0);
        }
    }

    update_xft_settings (this);
    start_fontconfig_monitor (this);
    sendSessionDbus();

    for (i = 0;  pManagers [i]; i++){
        pManagers [i]->set_string ( "Net/FallbackIconTheme", "ukui");
    }
    for (i = 0;  pManagers [i]; i++) {
        pManagers [i]->notify ( );
    }
    return true;
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
    return 1;
}
