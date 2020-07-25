#include <QDebug>
#include "keybindings-manager.h"
#include "config.h"
#include "clib-syslog.h"
#include "dconf-util.h"
#include <gio/gdesktopappinfo.h>
#include <QMessageBox>
#include <QScreen>


#define DESKTOP_APP_DIR "/usr/share/applications/"
#define GSETTINGS_KEYBINDINGS_DIR "/org/ukui/desktop/keybindings/"
#define CUSTOM_KEYBINDING_SCHEMA  "org.ukui.control-center.keybinding"

KeybindingsManager *KeybindingsManager::mKeybinding = nullptr;

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

/**
 * @brief parse_binding
 * Whether the binding exists
 * 获取此绑定的条目
 * @return
 */
static bool
parse_binding (Binding *binding)
{
    gboolean success;

    if(!binding)
        return false;

    binding->key.keysym = 0;
    binding->key.state = 0;
    g_free (binding->key.keycodes);
    binding->key.keycodes = NULL;

    if (binding->binding_str == NULL ||
        binding->binding_str[0] == '\0' ||
        g_strcmp0 (binding->binding_str, "Disabled") == 0 ||
        g_strcmp0 (binding->binding_str, "disabled") == 0 ) {
            return false;
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

/**
 * @brief KeybindingsManager::bindings_get_entry
 * Gets binding shortcut data
 * 获取绑定快捷键数据
 * @return
 */
bool KeybindingsManager::bindings_get_entry (KeybindingsManager *manager,const char *settings_path)
{
    GSettings *settings;
    Binding   *new_binding;
    GSList    *tmp_elem;
    char      *action = nullptr;
    char      *key    = nullptr;

    if (!settings_path) {
            return false;
    }

    /* Get entries for this binding
     * 获取此绑定的条目
     */
    settings = g_settings_new_with_path (CUSTOM_KEYBINDING_SCHEMA, settings_path);
    action = g_settings_get_string (settings, "action");
    key = g_settings_get_string (settings, "binding");
    g_object_unref (settings);

    if (action==nullptr || key==nullptr)
    {
            qWarning ("Key binding (%s) is incomplete", settings_path);
            return false;
    }

    tmp_elem = g_slist_find_custom (manager->binding_list,
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

    new_binding->binding_str = key;
    new_binding->action = action;
    new_binding->settings_path = g_strdup (settings_path);

    if (parse_binding (new_binding)) {
        if (!tmp_elem)
            manager->binding_list = g_slist_prepend (manager->binding_list, new_binding);
    } else {
        g_free (new_binding->binding_str);
        g_free (new_binding->action);
        g_free (new_binding->settings_path);
        g_free (new_binding->previous_key.keycodes);
        g_free (new_binding);
        if (tmp_elem)
            manager->binding_list = g_slist_delete_link (manager->binding_list, tmp_elem);
        return false;
    }

    return true;
}

/**
 * @brief KeybindingsManager::bindings_clear
 * Clear binding cache
 * 清除绑定缓存
 */
void KeybindingsManager::bindings_clear (KeybindingsManager *manager)
{
    GSList *l;

    if (manager->binding_list != NULL)
    {
        for (l = manager->binding_list; l; l = l->next) {
            Binding *b = (Binding *)l->data;
            g_free (b->binding_str);
            g_free (b->action);
            g_free (b->settings_path);
            g_free (b->previous_key.keycodes);
            g_free (b->key.keycodes);
            g_free (b);
        }
        g_slist_free (manager->binding_list);
        manager->binding_list = NULL;
    }
}

void KeybindingsManager::bindings_get_entries (KeybindingsManager *manager)
{
    gchar **custom_list = NULL;
    gint i;

    bindings_clear(manager);
    custom_list = dconf_util_list_subdirs (GSETTINGS_KEYBINDINGS_DIR, FALSE);

    if (custom_list != NULL)
    {
        for (i = 0; custom_list[i] != NULL; i++)
        {
            gchar *settings_path;
            settings_path = g_strdup_printf("%s%s", GSETTINGS_KEYBINDINGS_DIR, custom_list[i]);
            bindings_get_entry (manager,settings_path);
            g_free (settings_path);
        }
        g_strfreev (custom_list);
    }
}

/**
 * @brief same_keycode
 * Is it the same key code
 * 是否为相同的键码
 * @return
 */
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

/**
 * @brief same_key
 * Compare whether the keys are shortcuts
 * 对比按键是否为快捷键
 * @return
 */
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

/**
 * @brief KeybindingsManager::key_already_used
 * @English Compare the shortcuts already used
 * @简体 已经使用的快捷键 进行比对
 * @return
 */
bool KeybindingsManager::key_already_used (KeybindingsManager*manager,Binding   *binding)
{
    GSList *li;

    for (li = manager->binding_list; li != NULL; li = li->next) {
        Binding *tmp_binding =  (Binding*) li->data;

        if (tmp_binding != binding &&
            same_keycode (&tmp_binding->key, &binding->key) &&
            tmp_binding->key.state == binding->key.state) {
                return true;
        }
    }
    return false;
}

/**
 * @brief KeybindingsManager::binding_unregister_keys
 * Unbind key
 * 注销绑定按键
 */
void KeybindingsManager::binding_unregister_keys (KeybindingsManager *manager)
{
    GSList *li;
    bool need_flush = FALSE;
    gdk_x11_display_error_trap_push (gdk_display_get_default());
    try {
        for (li = manager->binding_list; li != NULL; li = li->next) {
            Binding *binding = (Binding *) li->data;

            if (binding->key.keycodes) {
                need_flush = TRUE;
                grab_key_unsafe (&binding->key, FALSE, manager->screens);
            }
        }
        if (need_flush)
            gdk_display_flush(gdk_display_get_default());
    } catch (...) {

    }
    gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
}

/**
 * @brief KeybindingsManager::binding_register_keys
 * Bind register key
 * 绑定寄存器按键
 */
void KeybindingsManager::binding_register_keys (KeybindingsManager *manager)
{
    GSList *li;
    bool need_flush = false;
    gdk_x11_display_error_trap_push (gdk_display_get_default());
    /* Now check for changes and grab new key if not already used
     * 现在检查更改并获取新密钥（如果尚未使用）
     */
    for (li = manager->binding_list; li != NULL; li = li->next) {
        Binding *binding = (Binding *) li->data;
        if (!same_key (&binding->previous_key, &binding->key)) {

            /* Ungrab key if it changed and not clashing with previously set binding */
            if (!key_already_used (manager,binding)) {
                gint i;
                need_flush = true;
                if (binding->previous_key.keycodes) {
                        grab_key_unsafe (&binding->previous_key, FALSE, manager->screens);
                }
                grab_key_unsafe (&binding->key, TRUE, manager->screens);
                binding->previous_key.keysym = binding->key.keysym;
                binding->previous_key.state = binding->key.state;
                g_free (binding->previous_key.keycodes);

                for (i = 0; binding->key.keycodes&&binding->key.keycodes[i]; ++i);
                binding->previous_key.keycodes = g_new0 (guint, i);

                for (i = 0; binding->key.keycodes&&binding->key.keycodes[i]; ++i)
                binding->previous_key.keycodes[i] = binding->key.keycodes[i];

            } else
                qDebug ("Key binding (%s) is already in use", binding->binding_str);
        }
    }

    if (need_flush)
        gdk_display_flush (gdk_display_get_default());
    if(gdk_x11_display_error_trap_pop (gdk_display_get_default()))
        qWarning("Grab failed for some keys, another application may already have access the them.");
}

/**
 * @brief keybindings_filter   键盘事件回调函数
 * @param gdk_xevent  GDK XEvent事件
 * @param event   GDK Event事件
 * @param manager 类
 * @return 未处理事件，请继续处理
 */
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

            if (binding->action == NULL)
                return GDK_FILTER_CONTINUE;

            if (!g_shell_parse_argv (binding->action,
                                     NULL, &argv,
                                     &error)) {
                    return GDK_FILTER_CONTINUE;
            }

            char execPathName[255] ;

            sprintf(execPathName, "%s%s", DESKTOP_APP_DIR, binding->action);
            GDesktopAppInfo *info = g_desktop_app_info_new_from_filename(execPathName);
            retval = g_app_info_launch_uris((GAppInfo *)info, NULL, NULL, NULL);

            g_strfreev (argv);

            /* Run failed popup
             * 运行失败弹窗 */
            if (!retval) {
                QString strs;
                strs.sprintf("\nError while trying to run (%s)\n\n which is linked to the key (%s)\n",binding->action,binding->binding_str);
                QMessageBox *msgbox = new QMessageBox();
                msgbox->setWindowTitle(QObject::tr("快捷键消息弹框"));
                msgbox->setText(QObject::tr(strs.toLatin1().data()));
                msgbox->setStandardButtons(QMessageBox::Yes);
                msgbox->setButtonText(QMessageBox::Yes,QObject::tr("确定"));
                msgbox->exec();
            }
            return GDK_FILTER_REMOVE;
        }
    }
    return GDK_FILTER_CONTINUE;
}

/**
 * @brief KeybindingsManager::bindings_callback  dconf监听快捷键改变回调函数
 * @param client
 * @param prefix
 * @param changes
 * @param tag
 * @param manager 类
 */
void KeybindingsManager::bindings_callback (DConfClient  *client,
                                            gchar        *prefix,
                                            GStrv        changes,
                                            gchar        *tag,
                                            KeybindingsManager *manager)
{
    qDebug ("keybindings: received 'changed' signal from dconf");

    binding_unregister_keys (manager);
    bindings_get_entries (manager);
    binding_register_keys (manager);
}


/**
 * @brief get_screens_list  获取gdkscreen屏幕并存放链表
 * @return  返回列表
 */
void KeybindingsManager::get_screens_list (void)
{
    GdkScreen  *screen = gdk_screen_get_default ();
    screens->append(screen);
}

bool KeybindingsManager::KeybindingsManagerStart()
{
    qDebug("Keybindings Manager Start");
    QList<GdkScreen*>::iterator l, begin, end;
    GdkDisplay  *dpy;
    GdkScreen   *screen;
    GdkWindow   *window;
    Display     *xdpy;
    Window       xwindow;
    XWindowAttributes atts;

    gdk_init(NULL,NULL);
    dpy = gdk_display_get_default ();

    xdpy = QX11Info::display();

    screen = gdk_display_get_default_screen(dpy);
    window  = gdk_screen_get_root_window (screen);
    xwindow = GDK_WINDOW_XID (window);

    /* Monitor keyboard events
     * 监听键盘事件
     */
    gdk_window_add_filter (window,
                           (GdkFilterFunc) keybindings_filter,
                           this);


    try {
        /* Add KeyPressMask to the currently reportable event masks
         * 将KeyPressMask添加到当前可报告的事件掩码
         */
        gdk_x11_display_error_trap_push (gdk_display_get_default());
        XGetWindowAttributes (xdpy, xwindow, &atts);
        XSelectInput (xdpy, xwindow, atts.your_event_mask | KeyPressMask);
        gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
    } catch (int) {

    }
    screens = new QList<GdkScreen*>();
    get_screens_list ();

    binding_list = NULL;
    bindings_get_entries (this);
    binding_register_keys(this);

    /* Link to dconf, receive a shortcut key change signal from dconf
     * 链接dconf, 从dconf收到更改快捷键信号
     */
    client = dconf_client_new ();
    dconf_client_watch_fast (client, GSETTINGS_KEYBINDINGS_DIR);
    g_signal_connect (client, "changed", G_CALLBACK (bindings_callback), this);

    return true;
}

void KeybindingsManager::KeybindingsManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"Stopping keybindings manager");

    if (client != NULL) {
            g_object_unref (client);
            client = NULL;
    }
    QList<GdkScreen*>::iterator l,end;

    l = screens->begin();
    GdkScreen *screen = *l;
    gdk_window_remove_filter (gdk_screen_get_root_window (screen),
                              (GdkFilterFunc) keybindings_filter,
                              this);


    binding_unregister_keys (this);
    bindings_clear (this);

    screens->clear();
    delete screens;
    screens = NULL;
}
