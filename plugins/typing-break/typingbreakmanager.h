#ifndef TYPINGBREAKMANAGER_H
#define TYPINGBREAKMANAGER_H

#include <glib-object.h>
#include <gio/gio.h>

#define UKUI_BREAK_SCHMA "org.mate.typing-break"

class TypingBreakManager
{
public:
    ~TypingBreakManager();
    static TypingBreakManager* TypingBreakManagerNew();
    bool TypingBreakManagerStart(GError** error);;
    void TypingBreakManagerStop();

private:
    TypingBreakManager();
    static gboolean typing_break_timeout();
    static void child_watch(int pid,int status);
    static void setup_typing_break(gboolean enabled);
    static void typing_break_enabled_callback(GSettings *settings,
                                              gchar* key,
                                              TypingBreakManager* manager);
    static bool really_setup_typing_break();

    int    typing_monitor_pid;
    guint   typing_monitor_idle_id;
    guint   child_watch_id;
    guint   setup_id;
    GSettings   *settings;

    static TypingBreakManager* mTypingBreakManager;
};

#endif // TYPINGBREAKMANAGER_H









