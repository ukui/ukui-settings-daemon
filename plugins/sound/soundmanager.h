#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QList>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#define UKUI_SOUND_SCHEMA "org.mate.sound"
#define PACKAGE_NAME "ukui-settings-daemon"
#define PACKAGE_VERSION "1.1.1"

class SoundManager{
public:
    ~SoundManager();
    static SoundManager* SoundManagerNew();
    bool SoundManagerStart(GError **error);
    void SoundManagerStop();
private:
    SoundManager();

    static bool flush_cb();
    void trigger_flush();
    static void gsettings_notify_cb (GSettings *client,char *key);
    static void file_monitor_changed_cb(GFileMonitor *monitor,
                                 GFile *file,
                                 GFile *other_file,
                                 GFileMonitorEvent event);
    bool register_directory_callback(const char *path,
                                         GError **error);

    static SoundManager* mSoundManager;
    GSettings *settings;
    QList<GFileMonitor*>* monitors2;//GFileMonitor-->QFileSystemWatcher ?
    unsigned int timeout;

};

#endif /* SOUNDMANAGER_H */
