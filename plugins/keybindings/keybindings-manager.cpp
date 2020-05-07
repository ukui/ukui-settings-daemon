#include "keybindings-manager.h"
#include "config.h"
#include "clib-syslog.h"
#include "dconf-util.h"
#include <QMessageBox>
#include <QScreen>

#define GSETTINGS_KEYBINDINGS_DIR "/org/ukui/desktop/keybindings/"
#define CUSTOM_KEYBINDING_SCHEMA  "org.ukui.control-center.keybinding"

KeybindingsManager *KeybindingsManager::mKeybinding = nullptr;
GSList *KeybindingsManager::binding_list = nullptr;
GSList *KeybindingsManager::screens = nullptr;

KeybindingsManager::KeybindingsManager()
{

}

KeybindingsManager::~KeybindingsManager()
{

}

KeybindingsManager *KeybindingsManager::KeybindingsManagerNew()
{
    if(nullptr == mKeybinding)
        mKeybinding = new KeybindingsManager();
    return mKeybinding;
}

static bool
parse_binding (Binding *binding)
{
    gboolean success;

    g_return_val_if_fail (binding != NULL, FALSE);

    binding->key.keysym = 0;
    binding->key.state = 0;
    g_free (binding->key.keycodes);
    binding->key.keycodes = NULL;

    if (binding->binding_str == NULL ||
        binding->binding_str[0] == '\0' ||
        g_strcmp0 (binding->binding_str, "Disabled") == 0 ||
        g_strcmp0 (binding->binding_str, "disabled") == 0 ) {
            return FALSE;
    }
    success = egg_accelerator_parse_virtual (binding->binding_str,
                                             &binding->key.keysym,
                                             &binding->key.keycodes,
                                             (EggVirtualModifierType *)&binding->key.state);

    if (!success)
        qWarning ("Key binding (%s) is invalid", binding->settings_path);

    return success;
}

static gint
compare_bindings (gconstpointer a,
                  gconstpointer b)
{
    Binding *key_a = (Binding *) a;
    char    *key_b = (char *) b;

    return g_strcmp0 (key_b, key_a->settings_path);
}

bool KeybindingsManager::bindings_get_entry (const char *settings_path)
{
    QGSettings *settings;
    Binding   *new_binding;
    GSList    *tmp_elem;
    QString   action = nullptr;
    QString   key    = nullptr;

    if (!settings_path) {
            return FALSE;
    }

    /* Get entries for this binding */
    settings = new QGSettings(CUSTOM_KEYBINDING_SCHEMA,settings_path);
    //settings = g_settings_new_with_path (CUSTOM_KEYBINDING_SCHEMA, settings_path);
    action = settings->get("action").toString();
    key = settings->get("binding").toString();
    delete settings;
    qDebug("action =%s key=%s",action.toLatin1().data(),key.toLatin1().data());
    if (!action.toLatin1().data() || !key.toLatin1().data() )
    {
            qWarning ("Key binding (%s) is incomplete", settings_path);
            return false;
    }

    qDebug ("keybindings: get entries from '%s' (action: '%s', key: '%s')",
            settings_path, action.toLatin1().data(), key.toLatin1().data());

    tmp_elem = g_slist_find_custom (binding_list,
                                    settings_path,
                                    compare_bindings);
    if (!tmp_elem) {
        new_binding = g_new0 (Binding, 1);
    } else {
        new_binding = (Binding *) tmp_elem->data;
        g_free (new_binding->binding_str);
        g_free (new_binding->action);
        g_free (new_binding->settings_path);

        new_binding->previous_key.keysym = new_binding->key.keysym;
        new_binding->previous_key.state = new_binding->key.state;
        new_binding->previous_key.keycodes = new_binding->key.keycodes;
        new_binding->key.keycodes = NULL;
    }

    new_binding->binding_str = key.toLatin1().data();
    new_binding->action = action.toLatin1().data();
    new_binding->settings_path = g_strdup (settings_path);

    if (parse_binding (new_binding)) {
        if (!tmp_elem)
            binding_list = g_slist_prepend (binding_list, new_binding);
    } else {
        g_free (new_binding->binding_str);
        g_free (new_binding->action);
        g_free (new_binding->settings_path);
        g_free (new_binding->previous_key.keycodes);
        g_free (new_binding);

        if (tmp_elem)
            binding_list = g_slist_delete_link (binding_list, tmp_elem);
        return FALSE;
    }

    return TRUE;
}

void KeybindingsManager::bindings_clear ()
{
    GSList *l;

    if (binding_list != NULL)
    {
        for (l = binding_list; l; l = l->next) {
            Binding *b = (Binding *)l->data;
            g_free (b->binding_str);
            g_free (b->action);
            g_free (b->settings_path);
            g_free (b->previous_key.keycodes);
            g_free (b->key.keycodes);
            g_free (b);
        }
        g_slist_free (binding_list);
        binding_list = NULL;
    }
}

void KeybindingsManager::bindings_get_entries ()
{
    gchar **custom_list = NULL;
    gint i;

    bindings_clear();
    custom_list = dconf_util_list_subdirs (GSETTINGS_KEYBINDINGS_DIR, FALSE);

    if (custom_list != NULL)
    {
        for (i = 0; custom_list[i] != NULL; i++)
        {
            gchar *settings_path;
            settings_path = g_strdup_printf("%s%s", GSETTINGS_KEYBINDINGS_DIR, custom_list[i]);
            bindings_get_entry (settings_path);
            g_free (settings_path);
        }
        g_strfreev (custom_list);
    }
}

static bool
same_keycode (const Key *key, const Key *other)
{
    if (key->keycodes != NULL && other->keycodes != NULL) {
        unsigned int *c;

        for (c = key->keycodes; *c; ++c) {
            if (key_uses_keycode (other, *c))
                return true;
        }
    }
    return false;
}

static bool same_key (const Key *key, const Key *other)
{
    if (key->state == other->state) {
        if (key->keycodes != NULL && other->keycodes != NULL) {
            unsigned int *c1, *c2;

            for (c1 = key->keycodes, c2 = other->keycodes;
                 *c1 || *c2; ++c1, ++c2) {
                         if (*c1 != *c2)
                            return false;
            }
        } else if (key->keycodes != NULL || other->keycodes != NULL)
            return false;

        return true;
    }
    return false;
}

bool KeybindingsManager::key_already_used (Binding   *binding)
{
    GSList *li;

    for (li = binding_list; li != NULL; li = li->next) {
        Binding *tmp_binding =  (Binding*) li->data;

        if (tmp_binding != binding &&
            same_keycode (&tmp_binding->key, &binding->key) &&
            tmp_binding->key.state == binding->key.state) {
                return true;
        }
    }
    return false;
}

void KeybindingsManager::binding_unregister_keys ()
{
    GSList *li;
    gboolean need_flush = FALSE;

    try {
        for (li = binding_list; li != NULL; li = li->next) {
            Binding *binding = (Binding *) li->data;

            if (binding->key.keycodes) {
                need_flush = TRUE;
                grab_key_unsafe (&binding->key, FALSE, screens);
            }
        }

        if (need_flush)
            gdk_flush ();

    } catch (...) {

    }


}

void KeybindingsManager::binding_register_keys ()
{
    GSList *li;
    gboolean need_flush = FALSE;

    /* Now check for changes and grab new key if not already used */
    for (li = binding_list; li != NULL; li = li->next) {
        Binding *binding = (Binding *) li->data;

        if (!same_key (&binding->previous_key, &binding->key)) {
            /* Ungrab key if it changed and not clashing with previously set binding */
            if (!key_already_used (binding)) {
                gint i;

                need_flush = TRUE;
                if (binding->previous_key.keycodes) {
                        grab_key_unsafe (&binding->previous_key, FALSE, screens);
                }
                grab_key_unsafe (&binding->key, TRUE, screens);

                binding->previous_key.keysym = binding->key.keysym;
                binding->previous_key.state = binding->key.state;
                g_free (binding->previous_key.keycodes);
                for (i = 0; binding->key.keycodes[i]; ++i);
                binding->previous_key.keycodes = g_new0 (guint, i);
                for (i = 0; binding->key.keycodes[i]; ++i)
                    binding->previous_key.keycodes[i] = binding->key.keycodes[i];
            } else
                qWarning ("Key binding (%s) is already in use", binding->binding_str);
        }
    }
    if (need_flush)
        gdk_flush ();

}

static char *
screen_exec_display_string (GdkScreen *screen)
{
    GString    *str;
    const char *old_display;
    char       *p;

    g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

    old_display = gdk_display_get_name (gdk_screen_get_display (screen));
    //QString screen_name = QScreen::name();
    str = g_string_new ("DISPLAY=");
    g_string_append (str, old_display);

    p = strrchr (str->str, '.');
    if (p && p >  strchr (str->str, ':')) {
        g_string_truncate (str, p - str->str);
    }

    g_string_append_printf (str, ".%d", gdk_screen_get_number (screen));

    return g_string_free (str, FALSE);
}

/**
 * get_exec_environment:
 *
 * Description: Modifies the current program environment to
 * ensure that $DISPLAY is set such that a launched application
 * inheriting this environment would appear on screen.
 *
 * Returns: a newly-allocated %NULL-terminated array of strings or
 * %NULL on error. Use g_strfreev() to free it.
 *
 * mainly ripped from egg_screen_exec_display_string in
 * ukui-panel/egg-screen-exec.c
 **/
static char **
get_exec_environment (XEvent *xevent)
{
    char     **retval = NULL;
    int        i;
    int        display_index = -1;
    GdkScreen *screen = NULL;
    GdkWindow *window = gdk_x11_window_lookup_for_display (gdk_display_get_default (), xevent->xkey.root);

    if (window) {
        screen = gdk_window_get_screen (window);
    }

    g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

    for (i = 0; environ [i]; i++) {
        if (!strncmp (environ [i], "DISPLAY", 7)) {
            display_index = i;
        }
    }

    if (display_index == -1) {
        display_index = i++;
    }
    retval = g_new (char *, i + 1);
    for (i = 0; environ [i]; i++) {
        if (i == display_index) {
            retval [i] = screen_exec_display_string (screen);
        } else {
            retval [i] = g_strdup (environ [i]);
        }
    }

    retval [i] = NULL;

    return retval;
}

GdkFilterReturn
keybindings_filter (GdkXEvent           *gdk_xevent,
                    GdkEvent            *event,
                    KeybindingsManager  *manager)
{
    XEvent *xevent = (XEvent *) gdk_xevent;
    GSList *li;
 
    if (xevent->type != KeyPress) {
        return GDK_FILTER_CONTINUE;
    }

    for (li = manager->binding_list; li != NULL; li = li->next) {
        Binding *binding = (Binding *) li->data;

        if (match_key (&binding->key, xevent)) {
            GError  *error = NULL;
            gboolean retval;
            gchar  **argv = NULL;
            gchar  **envp = NULL;

            g_return_val_if_fail (binding->action != NULL, GDK_FILTER_CONTINUE);

            if (!g_shell_parse_argv (binding->action,
                                     NULL, &argv,
                                     &error)) {
                    return GDK_FILTER_CONTINUE;
            }

            envp = get_exec_environment (xevent);

            retval = g_spawn_async (NULL,
                                    argv,
                                    envp,
                                    G_SPAWN_SEARCH_PATH,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &error);
            g_strfreev (argv);
            g_strfreev (envp);

            if (!retval) {
                GtkWidget *dialog;
                dialog = gtk_message_dialog_new(NULL, (GtkDialogFlags)0,
                                                GTK_MESSAGE_WARNING,
                                                GTK_BUTTONS_CLOSE,
                                                _("Error while trying to run (%s)\n"\
                                                  "which is linked to the key (%s)"),
                                                binding->action,
                                                binding->binding_str);
                g_signal_connect (dialog,
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  NULL);
                gtk_widget_show (dialog);
            }
            return GDK_FILTER_REMOVE;
        }
    }
    return GDK_FILTER_CONTINUE;
}

void KeybindingsManager::bindings_callback (DConfClient  *client,
                                            gchar        *prefix,
                                            GStrv        changes,
                                            gchar        *tag)
{
    qDebug ("keybindings: received 'changed' signal from dconf");

    binding_unregister_keys ();
    bindings_get_entries ();
    binding_register_keys ();
}

static GSList *
get_screens_list (void)
{
    GdkDisplay *display = gdk_display_get_default();
    int         n_screens;
    GSList     *list = NULL;
    int         i;

    n_screens = gdk_display_get_n_screens (display);

    if (n_screens == 1) {
        list = g_slist_append (list, gdk_screen_get_default ());
    } else {
        for (i = 0; i < n_screens; i++) {
            GdkScreen *screen;

            screen = gdk_display_get_screen (display, i);
            if (screen != NULL) {
                list = g_slist_prepend (list, screen);
            }
        }
        list = g_slist_reverse (list);
    }

    return list;
}

bool KeybindingsManager::KeybindingsManagerStart()
{
    CT_SYSLOG(LOG_DEBUG,"Keybindings Manager Start");

    GdkDisplay  *dpy;
    GdkScreen   *screen;
    GdkWindow   *window;

    int          screen_num;
    int          i;
    Display     *xdpy;
    Window       xwindow;
    XWindowAttributes atts;

    dpy = gdk_display_get_default ();
    xdpy = QX11Info::display();  // GDK_DISPLAY_XDISPLAY (dpy);
    screen_num = QX11Info::appScreen();//gdk_display_get_n_screens (dpy);

    for (i = 0; i < screen_num; i++) {
        screen = gdk_display_get_screen (dpy, i);
        xwindow = QX11Info::appRootWindow(i);//
        window  = gdk_screen_get_root_window (screen);
        //xwindow = GDK_WINDOW_XID (window);

        gdk_window_add_filter (window,
                               (GdkFilterFunc) keybindings_filter,
                               this);

        try {
            /* Add KeyPressMask to the currently reportable event masks */
            XGetWindowAttributes (xdpy, xwindow, &atts);
            XSelectInput (xdpy, xwindow, atts.your_event_mask | KeyPressMask);
        } catch (int) {

        }
    }
    screens = get_screens_list ();

    binding_list = NULL;
    bindings_get_entries ();
    binding_register_keys();

    client = dconf_client_new ();
    dconf_client_watch_fast (client, GSETTINGS_KEYBINDINGS_DIR);
    g_signal_connect (client, "changed", G_CALLBACK (bindings_callback), this);
    return true;
}

void KeybindingsManager::KeybindingsManagerStop()
{
    GSList *l;

    CT_SYSLOG(LOG_DEBUG,"Stopping keybindings manager");
    if (client != NULL) {
            g_object_unref (client);
            client = NULL;
    }

    for (l = screens; l; l = l->next) {
            GdkScreen *screen = (GdkScreen *)l->data;
            gdk_window_remove_filter (gdk_screen_get_root_window (screen),
                                      (GdkFilterFunc) keybindings_filter,
                                      this);
    }

    binding_unregister_keys ();
    bindings_clear ();

    g_slist_free (screens);
    screens = NULL;
}
