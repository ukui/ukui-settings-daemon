#ifndef MEDIAKEYSMANAGER_H
#define MEDIAKEYSMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QProcess>
#include <QGSettings>
#include <QDebug>
#include <QVariant>
#include <QFileInfo>
#include <QDir>

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

class MediaKeysManager:public QObject
{
    Q_OBJECT
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

private Q_SLOTS:
    //void timeoutCallback();
    void updateKbdCallback(const QString&);

private:
    static MediaKeysManager* mManager;
    QTimer            *mTimer;
    QGSettings        *mSettings;
    QList<GdkScreen*> *mScreenList;
    QProcess          *mExecCmd;
    GdkScreen         *mCurrentScreen;

    MateMixerStream   *mStream;
    MateMixerContext  *mContext;
    MateMixerStreamControl  *mControl;
    VolumeWindow      *mVolumeWindow;
    DeviceWindow      *mDeviceWindow;

};

#endif // MEDIAKEYSMANAGER_H
