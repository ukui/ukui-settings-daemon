#ifndef MEDIAKEYSMANAGER_H
#define MEDIAKEYSMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QProcess>
#include <QGSettings>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QDBusConnection>

#include "volumewindow.h"
#include "devicewindow.h"
#include "acme.h"

#ifdef signals
#undef signals
#endif

extern "C"{
#include <syslog.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libmatemixer/matemixer.h>
#include <X11/Xlib.h>
#include "ukui-input-helper.h"
}

/** this value generate from mpris serviceRegisteredSlot(const QString&)
 *  这个值一般由mpris插件的serviceRegisteredSlot(const QString&)传递
 */
typedef struct{
    QString application;
    uint time;
}MediaPlayer;

class MediaKeysManager:public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.MediaKeys")
public:
    ~MediaKeysManager();
    static MediaKeysManager* mediaKeysNew();
    bool mediaKeysStart(GError*);
    void mediaKeysStop();

private:
    MediaKeysManager(QObject* parent = nullptr);
    void initScreens();
    void initKbd();

    static GdkFilterReturn acmeFilterEvents(GdkXEvent*,GdkEvent*,void*);
    static void onContextStateNotify(MateMixerContext*,GParamSpec*,void*);
    static void onContextDefaultOutputNotify(MateMixerContext*,GParamSpec*,void*);
    static void onContextStreamRemoved(MateMixerContext*,char*,void*);
    static void updateDefaultOutput();
    GdkScreen *acmeGetScreenFromEvent (XAnyEvent*);
    bool doAction(int);

    /******************Functional class function(功能类函数)****************/
    void doTouchpadAction();
    void doSoundAction(int);
    void updateDialogForVolume(uint,bool,bool);
    void executeCommand(const QString&,const QString&);
    void doShutdownAction();
    void doLogoutAction();
    void doOpenHomeDirAction();
    void doSearchAction();
    void doScreensaverAction();
    void doSettingsAction();
    void doOpenFileManagerAction();
    void doMediaAction();
    void doOpenCalcAction();
    void doToggleAccessibilityKey(const QString key);
    void doMagnifierAction();
    void doScreenreaderAction();
    void doOnScreenKeyboardAction();
    void doOpenTerminalAction();
    void doScreenshotAction(const QString);
    void doUrlAction(const QString);
    void doMultiMediaPlayerAction(const QString);

    /******************Function for DBus(DBus相关处理函数)******************************/
    bool findMediaPlayerByApplication(const QString&);
    uint findMediaPlayerByTime(MediaPlayer*);
    void removeMediaPlayerByApplication(const QString&,uint);

public Q_SLOTS:
    /** two dbus method, will be called in mpris plugin(mprismanager.cpp MprisManagerStart())
     *  两个dbus 方法，将会在mpris插件中被调用(mprismanager.cpp MprisManagerStart())
     */
    void GrabMediaPlayerKeys(QString application);
    void ReleaseMediaPlayerKeys(QString application);

private Q_SLOTS:
    //void timeoutCallback();
    void updateKbdCallback(const QString&);

Q_SIGNALS:
    /** media-keys plugin will emit this signal by org.ukui.SettingsDaemon.MediaKeys
     *  when listen some key Pressed(such as XF86AudioPlay 、XF86AudioPause 、XF86AudioForward)
     *  at the same time,mpris plugin will connect this signal
     *
     *  当监听到某些按键(XF86AudioRewind、XF86AudioRepeat、XF86AudioRandomPlay)时，
     *  media-keys插件将会通过org.ukui.SettingsDaemon.MediaKeys这个dbus发送这个signal
     *  同时，mpris插件会connect这个signal
    */
    void MediaPlayerKeyPressed(QString application,QString operation);

private:
    static MediaKeysManager* mManager;
    QTimer            *mTimer;
    QGSettings        *mSettings;
    QList<GdkScreen*> *mScreenList;     //GdkSCreen list
    QProcess          *mExecCmd;
    GdkScreen         *mCurrentScreen;  //current GdkScreen

    MateMixerStream   *mStream;
    MateMixerContext  *mContext;
    MateMixerStreamControl  *mControl;
    VolumeWindow      *mVolumeWindow;   //volume size window 声音大小窗口
    DeviceWindow      *mDeviceWindow;   //other widow，such as touchapad、volume 例如触摸板、磁盘卷设备
    QList<MediaPlayer*> mediaPlayers;   //all opened media player(vlc,audacious) 已经打开的媒体播放器列表(vlc,audacious)

};

#endif // MEDIAKEYSMANAGER_H
