#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QFileSystemWatcher>
#include "QGSettings/qgsettings.h"
//#include <QGSettings>

#ifdef signals
#undef signals
#endif

extern "C"{
#include <gio/gio.h>
}

#define UKUI_SOUND_SCHEMA "org.mate.sound"
#define PACKAGE_NAME "ukui-settings-daemon"
#define PACKAGE_VERSION "1.1.1"

class SoundManager : public QObject{
    Q_OBJECT
public:
    ~SoundManager();
    static SoundManager* SoundManagerNew();
    bool SoundManagerStart(GError **error);
    void SoundManagerStop();
private:
    SoundManager();

    void trigger_flush();
    bool register_directory_callback(const QString path,
                                         GError **error);

private Q_SLOTS:
    bool flush_cb();
    void gsettings_notify_cb (const QString& key);
    void file_monitor_changed_cb(const QString& path);

private:
    static SoundManager* mSoundManager;
    QGSettings* settings;
    QList<QFileSystemWatcher*>* monitors;
    QTimer* timer;
};

#endif /* SOUNDMANAGER_H */
