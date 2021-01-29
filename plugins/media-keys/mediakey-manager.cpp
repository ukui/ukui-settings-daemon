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

#include "mediakey-manager.h"
#include "eggaccelerators.h"

MediaKeysManager* MediaKeysManager::mManager = nullptr;

const int VOLUMESTEP = 6;
#define midValue(x,low,high) (((x) > (high)) ? (high): (((x) < (low)) ? (low) : (x)))

#define MEDIAKEY_SCHEMA "org.ukui.SettingsDaemon.plugins.media-keys"

#define POINTER_SCHEMA  "org.ukui.SettingsDaemon.plugins.mouse"
#define POINTER_KEY     "locate-pointer"

#define SESSION_SCHEMA  "org.ukui.session"
#define WIN_KEY         "win-key-release"

MediaKeysManager::MediaKeysManager(QObject* parent):QObject(parent)
{
    gdk_init(NULL,NULL);
    //session bus 会话总线
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if(sessionBus.registerService("org.ukui.SettingsDaemon")){
        sessionBus.registerObject("/org/ukui/SettingsDaemon/MediaKeys",this,
                                  QDBusConnection::ExportAllContents);
    }
}

MediaKeysManager::~MediaKeysManager()
{
}

MediaKeysManager* MediaKeysManager::mediaKeysNew()
{
    if(nullptr == mManager)
        mManager = new MediaKeysManager();
    return mManager;
}

bool MediaKeysManager::mediaKeysStart(GError*)
{
    mate_mixer_init();
    QList<GdkScreen*>::iterator l,begin,end;

    mSettings = new QGSettings(MEDIAKEY_SCHEMA);
    pointSettings = new QGSettings(POINTER_SCHEMA);
    sessionSettings = new QGSettings(SESSION_SCHEMA);

    mScreenList = new QList<GdkScreen*>();
    mVolumeWindow = new VolumeWindow();
    mDeviceWindow = new DeviceWindow();
    mExecCmd = new QProcess();
    mManager->mStream = NULL;
    mManager->mControl = NULL;
    mManager->mInputControl = NULL;
    mManager->mInputStream  = NULL;

    mVolumeWindow->initWindowInfo();
    mDeviceWindow->initWindowInfo();


    if(mate_mixer_is_initialized()){
        mContext = mate_mixer_context_new();
        g_signal_connect(mContext,"notify::state",
                         G_CALLBACK(onContextStateNotify),this);
        g_signal_connect(mContext,"notify::default-output-stream",
                         G_CALLBACK(onContextDefaultOutputNotify),this);
        g_signal_connect(mContext,"notify::removed",
                         G_CALLBACK(onContextStreamRemoved),this);

        mate_mixer_context_open(mContext);
    }

    initScreens();
    initKbd();
    initXeventMonitor();
    l = begin = mScreenList->begin();
    end = mScreenList->end();
    for(; l != end; ++l){
        //syslog(LOG_DEBUG,"adding key filter for screen: %d",gdk_screen_get_number(*l));
        gdk_window_add_filter(gdk_screen_get_root_window(*l),
                                 (GdkFilterFunc)acmeFilterEvents,
                                 NULL);
    }

    return true;
}

void MediaKeysManager::mediaKeysStop()
{
    QList<GdkScreen*>::iterator l,end;
    bool needFlush;
    int i;

    syslog(LOG_DEBUG,"Stooping media keys manager!");

    delete mSettings;
    mSettings = nullptr;
    delete pointSettings;
    pointSettings = nullptr;
    delete sessionSettings;
    sessionSettings = nullptr;
    delete mExecCmd;
    mExecCmd = nullptr;
    delete mVolumeWindow;
    mVolumeWindow = nullptr;
    delete mDeviceWindow;
    mDeviceWindow = nullptr;

    XEventMonitor::instance()->exit();

    l = mScreenList->begin();
    end = mScreenList->end();
    for(; l != end; ++l)
        gdk_window_remove_filter(gdk_screen_get_root_window(*l),
                                 (GdkFilterFunc)acmeFilterEvents,
                                 NULL);
    mScreenList->clear();
    delete  mScreenList;
    mScreenList = nullptr;

    needFlush = false;
    gdk_x11_display_error_trap_push(gdk_display_get_default());
    for(i = 0; i < HANDLED_KEYS; ++i){
        if(keys[i].key){
            needFlush = true;
            grab_key_unsafe(keys[i].key,false,mScreenList);
            g_free(keys[i].key->keycodes);
            g_free(keys[i].key);
            keys[i].key = NULL;
        }
    }

    if(needFlush)
        gdk_display_flush(gdk_display_get_default());

    gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());

    g_clear_object(&mStream);
    g_clear_object(&mControl);
    g_clear_object(&mInputControl);
    g_clear_object(&mInputStream);
    g_clear_object(&mContext);
}

void MediaKeysManager::XkbEventsRelease(const QString &keyStr)
{
    QString KeyName;
    static bool winFlag = false;
    static bool ctrlFlag = false;
    if (keyStr.compare("Print") == 0){
        executeCommand("kylin-screenshot", " full");
        return;
    }

    if(keyStr.compare("Control_L+Shift_L+Escape") == 0 || 
       keyStr.compare("Shift_L+Control_L+Escape") == 0 ){
        doOpenMonitor();
        return;
    }

    if (keyStr.length() >= 8)
        KeyName = keyStr.left(8);

    if(KeyName.compare("Super_L+") == 0 ||
       KeyName.compare("Super_R+") == 0 )
        winFlag = true;

    if ((winFlag && keyStr.compare("Super_L") == 0 )||
        (winFlag && keyStr.compare("Super_R") == 0 )){
        winFlag = false;
        return;
    } else if((m_winFlag && keyStr.compare("Super_L") == 0 )||
              (m_winFlag && keyStr.compare("Super_R") == 0 ))
            return;

    if (keyStr.length() >= 10)
        KeyName = keyStr.left(10);

    if(KeyName.compare("Control_L+") == 0 ||
       KeyName.compare("Control_R+") == 0 )
        ctrlFlag = true;

    if((ctrlFlag && keyStr.compare("Control_L") == 0 )||
       (ctrlFlag && keyStr.compare("Control_R") == 0 )){
        ctrlFlag = false;
        return;
    } else if((m_ctrlFlag && keyStr.compare("Control_L") == 0 )||
              (m_ctrlFlag && keyStr.compare("Control_R") == 0 ))
            return;

    if(keyStr.compare("Super_L") == 0 ||
       keyStr.compare("Super_R") == 0 ){
        if (!sessionSettings->get(WIN_KEY).toBool())
            executeCommand("ukui-menu", nullptr);
    }
    if (keyStr.compare("Control_L") == 0 ||
        keyStr.compare("Control_R") == 0)
        pointSettings->set("locate-pointer", !pointSettings->get(POINTER_KEY).toBool());
}

void MediaKeysManager::XkbEventsPress(const QString &keyStr)
{
    QString KeyName;

    if (keyStr.length() >= 8)
        KeyName = keyStr.left(8);

    if(KeyName.compare("Super_L+") == 0 ||
       KeyName.compare("Super_R+") == 0 )
        m_winFlag = true;

    if(m_winFlag && keyStr.compare("Super_L") == 0 ||
       m_winFlag && keyStr.compare("Super_R") == 0 ){
        m_winFlag = false;
        return;
    }

    if (keyStr.length() >= 10)
        KeyName = keyStr.left(10);

    if(KeyName.compare("Control_L+") == 0 ||
       KeyName.compare("Control_R+") == 0 )
        m_ctrlFlag = true;

    if(m_ctrlFlag && keyStr.compare("Control_L") == 0 ||
       m_ctrlFlag && keyStr.compare("Control_R") == 0 ){
        m_ctrlFlag = false;
        return;
    }
}

void MediaKeysManager::initXeventMonitor()
{
    XEventMonitor::instance()->start();
    connect(XEventMonitor::instance(), SIGNAL(keyRelease(QString)),
            this, SLOT(XkbEventsRelease(QString)));
    connect(XEventMonitor::instance(), SIGNAL(keyPress(QString)),
            this, SLOT(XkbEventsPress(QString)));
}

void MediaKeysManager::initScreens()
{
    GdkDisplay *display;
    GdkScreen  *screen;

    display = gdk_display_get_default();

    screen = gdk_display_get_default_screen(display);
    if(NULL != screen)
        mScreenList->append(screen);

    if(mScreenList->count() > 0)
        mCurrentScreen = mScreenList->at(0);
    else
        mCurrentScreen = NULL;
}

GdkFilterReturn
MediaKeysManager::acmeFilterEvents(GdkXEvent* xevent,GdkEvent* event,void* data)
{
    XEvent    *xev = (XEvent *) xevent;
    XAnyEvent *xany = (XAnyEvent *) xevent;
    int       i;

    /* verify we have a key event */
    if (xev->type != KeyPress && xev->type != KeyRelease)
        return GDK_FILTER_CONTINUE;

    for (i = 0; i < HANDLED_KEYS; i++) {
        if (match_key (keys[i].key, xev)) {
            switch (keys[i].key_type) {
            case VOLUME_DOWN_KEY:
            case VOLUME_UP_KEY:
                /* auto-repeatable keys */
                if (xev->type != KeyPress)
                    return GDK_FILTER_CONTINUE;
                break;
            default:
                if (xev->type != KeyRelease)
                    return GDK_FILTER_CONTINUE;
            }

            mManager->mCurrentScreen = mManager->acmeGetScreenFromEvent(xany);

            if (mManager->doAction(keys[i].key_type) == false)
                return GDK_FILTER_REMOVE;
            else
                return GDK_FILTER_CONTINUE;
        }
    }

    return GDK_FILTER_CONTINUE;
}

void MediaKeysManager::onContextStateNotify(MateMixerContext *context,
                                            GParamSpec       *pspec,
                                            MediaKeysManager *manager)
{
    updateDefaultOutput(manager);
}

void MediaKeysManager::onContextDefaultOutputNotify(MateMixerContext *context,
                                                    GParamSpec       *pspec,
                                                    MediaKeysManager *manager)
{
    updateDefaultOutput(manager);
}

void MediaKeysManager::onContextStreamRemoved(MateMixerContext *context,
                                              char             *name,
                                              MediaKeysManager *mManager)
{
    if (mManager->mStream != NULL) {
        MateMixerStream *stream =
            mate_mixer_context_get_stream (mManager->mContext, name);

        if (stream == mManager->mStream) {
            g_clear_object (&mManager->mStream);
            g_clear_object (&mManager->mControl);
        }
    }
}

void MediaKeysManager::updateDefaultOutput(MediaKeysManager *mManager)
{
    MateMixerStream        *stream;
    MateMixerStreamControl *control = NULL;
    MateMixerStream        *inputStream;
    MateMixerStreamControl *inputControl = NULL;

    stream = mate_mixer_context_get_default_output_stream (mManager->mContext);
    if (stream != NULL)
        control = mate_mixer_stream_get_default_control (stream);

    inputStream = mate_mixer_context_get_default_input_stream (mManager->mContext);
    if (inputStream != NULL)
        inputControl = mate_mixer_stream_get_default_control (inputStream);

    if (stream == mManager->mStream || inputStream == mManager->mInputStream)
           return;

   	g_clear_object (&mManager->mStream);
   	g_clear_object (&mManager->mControl);
    g_clear_object (&mManager->mInputStream);
    g_clear_object (&mManager->mInputControl);
   
    if (control != NULL) {
            MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (control);

           /* Do not use the stream if it is not possible to mute it or
            * change the volume */
           if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
               !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                   return;

           mManager->mStream = stream;
           mManager->mControl = control;
           qDebug ("Default output stream updated to %s",
                    mate_mixer_stream_get_name (stream));
   } else
           qDebug ("Default output stream unset");

   if (inputControl != NULL) {
            MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (inputControl);

           /* Do not use the stream if it is not possible to mute it or
            * change the volume */
           if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
               !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                   return;

           mManager->mInputStream = inputStream;
           mManager->mInputControl = inputControl;
           qDebug ("Default input stream updated to %s",
                    mate_mixer_stream_get_name (inputStream));
   } else
           qDebug ("Default input stream unset");

}

GdkScreen *
MediaKeysManager::acmeGetScreenFromEvent (XAnyEvent *xanyev)
{
    GdkWindow *window;
    GdkScreen *screen;
    QList<GdkScreen*>::iterator l,begin,end;

    l = begin = mScreenList->begin();
    end = mScreenList->end();
    /* Look for which screen we're receiving events */
    for (; l != end; ++l) {
            screen = *l;
            window = gdk_screen_get_root_window (screen);

            if (GDK_WINDOW_XID (window) == xanyev->window)
                return screen;
    }
    return NULL;
}

bool MediaKeysManager::doAction(int type)
{
    switch(type){
    case TOUCHPAD_KEY:
        doTouchpadAction();
        break;
    case MUTE_KEY:
    case VOLUME_DOWN_KEY:
    case VOLUME_UP_KEY:
        doSoundAction(type);
        break;
    case MIC_MUTE_KEY:
        doMicSoundAction();
        break;
    case POWER_KEY:
        doShutdownAction();
        break;
    case LOGOUT_KEY:
        doLogoutAction();
        break;
    case EJECT_KEY:
        break;
    case HOME_KEY:
        doOpenHomeDirAction();
        break;
    case SEARCH_KEY:
        doSearchAction();
        break;
    case EMAIL_KEY:
        doUrlAction("mailto");
        break;
    case SCREENSAVER_KEY:
    case SCREENSAVER_KEY_2:
        doScreensaverAction();
        break;
    case SETTINGS_KEY:
    case SETTINGS_KEY_2:
        doSettingsAction();
        break;
    case WINDOWSWITCH_KEY:
    case WINDOWSWITCH_KEY_2:
        doWindowSwitchAction();
        break;
    case FILE_MANAGER_KEY:
    case FILE_MANAGER_KEY_2:
        doOpenFileManagerAction();
        break;
    case HELP_KEY:
        doUrlAction("help");
        break;
    case WWW_KEY:
        doUrlAction("http");
        break;
    case MEDIA_KEY:
        doMediaAction();
        break;
    case CALCULATOR_KEY:
        doOpenCalcAction();
        break;
    case PLAY_KEY:
        doMultiMediaPlayerAction("Play");
        break;
    case PAUSE_KEY:
        doMultiMediaPlayerAction("Pause");
        break;
    case STOP_KEY:
        doMultiMediaPlayerAction("Stop");
        break;
    case PREVIOUS_KEY:
        doMultiMediaPlayerAction("Previous");
        break;
    case NEXT_KEY:
        doMultiMediaPlayerAction("Next");
        break;
    case REWIND_KEY:
        doMultiMediaPlayerAction("Rewind");
        break;
    case FORWARD_KEY:
        doMultiMediaPlayerAction("FastForward");
        break;
    case REPEAT_KEY:
        doMultiMediaPlayerAction("Repeat");
        break;
    case RANDOM_KEY:
        doMultiMediaPlayerAction("Shuffle");
        break;
    case MAGNIFIER_KEY:
        doMagnifierAction();
        break;
    case SCREENREADER_KEY:
        doScreensaverAction();
        break;
    case ON_SCREEN_KEYBOARD_KEY:
        doOnScreenKeyboardAction();
        break;
    case TERMINAL_KEY:
    case TERMINAL_KEY_2:
        doOpenTerminalAction();
        break;
    case SCREENSHOT_KEY:
        doScreenshotAction(" full");
        break;
    case AREA_SCREENSHOT_KEY:
        doScreenshotAction(" gui");
        break;
    case WINDOW_SCREENSHOT_KEY:
        doScreenshotAction(" screen");
        break;
    case SYSTEM_MONITOR_KEY:
        doOpenMonitor();
        break;
    case CONNECTION_EDITOR_KEY:
        doOpenConnectionEditor();
        break;
    case GLOBAL_SEARCH_KEY:
        doOpenUkuiSearchAction();
        break;
    case KDS_KEY:
        doOpenKdsAction();
        break;
    default:
        break;
    }
    return false;
}

bool isValidShortcut (const QString& string)
{
    if (string.isNull() || string.isEmpty())
        return false;
    if (string == "disabled")
        return false;

    return true;
}

void MediaKeysManager::initKbd()
{
    int i;
    bool needFlush = false;

    gdk_x11_display_error_trap_push(gdk_display_get_default());
    connect(mSettings,SIGNAL(changed(const QString&)),this,SLOT(updateKbdCallback(const QString&)));

    for(i = 0; i < HANDLED_KEYS; ++i){
        QString tmp,schmeasKey;
        Key* key;

        if(NULL != keys[i].settings_key){
            schmeasKey = keys[i].settings_key;
            tmp = mSettings->get(schmeasKey).toString();
        }else
            tmp = keys[i].hard_coded;

        if(!isValidShortcut(tmp)){
            syslog(LOG_DEBUG,"Not a valid shortcut: '%s'(%s %s)",tmp.toLatin1().data(),
                   keys[i].settings_key,keys[i].hard_coded);
            tmp.clear();
            continue;
        }

        key = g_new0(Key,1);
        if(!egg_accelerator_parse_virtual(tmp.toLatin1().data(),&key->keysym,&key->keycodes,
                                          (EggVirtualModifierType*)&key->state)){
            syslog(LOG_DEBUG,"Unable to parse: '%s'",tmp.toLatin1().data());
            tmp.clear();
            g_free(key);
            continue;
        }

        tmp.clear();
        keys[i].key = key;
        needFlush = true;
        grab_key_unsafe(key,true,mScreenList);
    }

    if(needFlush)
        gdk_display_flush(gdk_display_get_default());
    if(gdk_x11_display_error_trap_pop(gdk_display_get_default()))
        syslog(LOG_WARNING,"Grab failed for some keys,another application may already have access the them.");
}

void MediaKeysManager::updateKbdCallback(const QString &key)
{
    int i;
    bool needFlush = true;

    if(key.isNull())
        return;

    gdk_x11_display_error_trap_push (gdk_display_get_default());

    /* Find the key that was modified */
    for (i = 0; i < HANDLED_KEYS; i++) {
        if (0 == key.compare(keys[i].settings_key)) {
            QString tmp;
            Key  *key;

            if (NULL != keys[i].key) {
                needFlush = true;
                grab_key_unsafe (keys[i].key, false, mScreenList);
            }

            g_free (keys[i].key);
            keys[i].key = NULL;

            /* We can't have a change in a hard-coded key */
            if(NULL != keys[i].settings_key){
                syslog(LOG_DEBUG,"settings key value is NULL,exit!");
                //return;
            }

            tmp = mSettings->get(keys[i].settings_key).toString();

            if (false == isValidShortcut(tmp)) {
                tmp.clear();
                break;
            }
            key = g_new0 (Key, 1);

            if (!egg_accelerator_parse_virtual (tmp.toLatin1().data(), &key->keysym, &key->keycodes,
                                                (EggVirtualModifierType*)&key->state)) {
                tmp.clear();
                g_free (key);
                break;
            }

            needFlush = true;
            grab_key_unsafe (key, true, mScreenList);
            keys[i].key = key;

            tmp.clear();
            break;
        }
    }

    if (needFlush)
        gdk_display_flush (gdk_display_get_default());
    if (gdk_x11_display_error_trap_pop (gdk_display_get_default()))
        syslog(LOG_WARNING,"Grab failed for some keys, another application may already have access the them.");
}

void MediaKeysManager::doTouchpadAction()
{
    QGSettings *touchpadSettings;
    bool touchpadState;

    touchpadSettings = new QGSettings("org.ukui.peripherals-touchpad");
    touchpadState = touchpadSettings->get("touchpad-enabled").toBool();
    if(FALSE == touchpad_is_present()){
        mDeviceWindow->setAction("touchpad-disabled");
        return;
    }
    mDeviceWindow->setAction(!touchpadState ? "touchpad-enabled" : "touchpad-disabled");
    mDeviceWindow->dialogShow();

    touchpadSettings->set("touchpad-enabled",!touchpadState);
    delete touchpadSettings;
}

void MediaKeysManager::doMicSoundAction()
{
    bool mute;
    mute = mate_mixer_stream_control_get_mute (mInputControl);
    mate_mixer_stream_control_set_mute(mInputControl, !mute);
    mDeviceWindow->setAction (mute ? "audio-input-microphone-high-symbolic" : "audio-input-microphone-muted-symbolic");
    mDeviceWindow->dialogShow();
}

void MediaKeysManager::doSoundAction(int keyType)
{
    bool muted,mutedLast,soundChanged;  //是否静音，上一次值记录，是否改变
    int volume,volumeMin,volumeMax;    //当前音量值，最小音量值，最大音量值
    uint volumeStep,volumeLast;         //音量步长，上一次音量值

    if(NULL == mControl)
        return;

    volumeMin = mate_mixer_stream_control_get_min_volume(mControl);
    volumeMax = mate_mixer_stream_control_get_normal_volume(mControl);
    volumeStep = mSettings->get("volume-step").toInt();
    if(volumeStep <= 0 || volumeStep > 100)
        volumeStep = VOLUMESTEP;

    volume = volumeLast = mate_mixer_stream_control_get_volume(mControl);
    muted = mutedLast = mate_mixer_stream_control_get_mute(mControl);

    switch(keyType){
    case MUTE_KEY:
        if(volume == volumeMin)
            muted = true;
        else
            muted = !muted;
        break;
    case VOLUME_DOWN_KEY:
        if(volume <= (volumeMin + volumeStep)){
            volume = volumeMin;
            muted = true;
        }else{
            volume -= volumeStep * 400;
            muted = false;
        }
        if(volume < 300){
            volume = volumeMin;
            muted = true;
        }
        break;
    case VOLUME_UP_KEY:
        if(muted){
            muted = false;
            if(volume <= (volumeMin + volumeStep))
                volume = volumeMin + volumeStep * 400;
        }else
            volume = midValue(volume + volumeStep * 400, volumeMin, volumeMax);
        break;
    }

    if(muted != mutedLast){
        if(mate_mixer_stream_control_set_mute(mControl, muted))
            soundChanged = true;
        else
            muted = mutedLast;
    }
    if(mate_mixer_stream_control_get_volume(mControl) != volume){
        if(mate_mixer_stream_control_set_volume(mControl,volume))
            soundChanged = true;
        else
            volume = volumeLast;
    }
    mVolumeWindow->setVolumeRange(volumeMin,volumeMax);
    updateDialogForVolume(volume,muted,soundChanged);
}

void MediaKeysManager::updateDialogForVolume(uint volume,bool muted,bool soundChanged)
{
    mVolumeWindow->setVolumeMuted(muted);
    mVolumeWindow->setVolumeLevel(volume);
    mVolumeWindow->dialogShow();
    //ToDo...
}

void processAbstractPath(QString& process)
{
    QString tmpPath;
    QFileInfo fileInfo;

    tmpPath = "/usr/bin/" + process;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists()){
        process = tmpPath;
        return;
    }

    tmpPath.clear();
    tmpPath = "/usr/sbin/" + process;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists()){
        process = tmpPath;
        return;
    }

    process = "";
}
bool binaryFileExists(const QString& binary)
{
    QString tmpPath;
    QFileInfo fileInfo;

    tmpPath = "/usr/bin/" + binary;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists())
        return true;

    tmpPath.clear();
    tmpPath = "/usr/sbin/" + binary;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists())
        return true;

    return false;
}

void MediaKeysManager::executeCommand(const QString& command,const QString& paramter){
    QString cmd = command + paramter;
    char   **argv;
    int     argc;
    bool    retval;

    //processAbstractPath(cmd);
    if(!cmd.isEmpty()){
        if (g_shell_parse_argv (cmd.toLatin1().data(), &argc, &argv, NULL)) {
            retval = g_spawn_async (g_get_home_dir (),
                                    argv,
                                    NULL,
                                    G_SPAWN_SEARCH_PATH,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
            g_strfreev (argv);
        }
        //mExecCmd->execute(cmd + paramter);//mExecCmd->start(cmd + paramter);
    }
    else
        syslog(LOG_DEBUG,"%s cannot found at system path!",command.toLatin1().data());
}

void MediaKeysManager::doShutdownAction()
{
    executeCommand("ukui-session-tools"," --shutdown");
}

void MediaKeysManager::doLogoutAction()
{
    executeCommand("ukui-session-tools","");
}

void MediaKeysManager::doOpenHomeDirAction()
{
    QString homePath;

    homePath = QDir::homePath();
    executeCommand("peony"," --show-folders " + homePath);
}

void MediaKeysManager::doSearchAction()
{
    QString tool1,tool2,tool3;

    tool1 = "beagle-search";
    tool2 = "tracker-search-tool";
    tool3 = "mate-search-tool";

    if(binaryFileExists(tool1)){
        executeCommand(tool1,"");
    }else if(binaryFileExists(tool2)){
        executeCommand(tool2,"");
    }else
        executeCommand(tool3,"");
}

void MediaKeysManager::doScreensaverAction()
{
    QString tool1,tool2;

    tool1 = "ukui-screensaver-command";
    tool2 = "xscreensaver-command";

    if(binaryFileExists(tool1))
        executeCommand(tool1," --lock");
    else
        executeCommand(tool2," --lock");
}

void MediaKeysManager::doSettingsAction()
{
    executeCommand("ukui-control-center","");
}

void MediaKeysManager::doWindowSwitchAction()
{
    executeCommand("ukui-window-switch"," --show-workspace");
}

void MediaKeysManager::doOpenFileManagerAction()
{
    executeCommand("peony","");
}

void MediaKeysManager::doMediaAction()
{
}

void MediaKeysManager::doOpenCalcAction()
{
    QString tool1,tool2,tool3;

    tool1 = "galculator";
    tool2 = "mate-calc";
    tool3 = "kylin-calculator";

    if(binaryFileExists(tool1)){
        executeCommand(tool1,"");
    }else if(binaryFileExists(tool2)){
        executeCommand(tool2,"");
    }else
        executeCommand(tool3,"");
}

void MediaKeysManager::doToggleAccessibilityKey(const QString key)
{
    QGSettings* toggleSettings;
    bool state;

    toggleSettings = new QGSettings("org.gnome.desktop.a11y.applications");
    state = toggleSettings->get(key).toBool();
    toggleSettings->set(key,!state);

    delete toggleSettings;
}

void MediaKeysManager::doMagnifierAction()
{
    doToggleAccessibilityKey("screen-magnifier-enabled");
}

void MediaKeysManager::doScreenreaderAction()
{
    doToggleAccessibilityKey("screen-reader-enabled");
}

void MediaKeysManager::doOnScreenKeyboardAction()
{
    doToggleAccessibilityKey("screen-keyboard-enabled");
}

void MediaKeysManager::doOpenTerminalAction()
{
    executeCommand("mate-terminal","");
}

void MediaKeysManager::doScreenshotAction(const QString pramater)
{
    executeCommand("kylin-screenshot",pramater);
}

void MediaKeysManager::doSidebarAction()
{
    executeCommand("ukui-sidebar"," -show");
}

void MediaKeysManager::doOpenMonitor()
{
    executeCommand("ukui-system-monitor","");
}

void MediaKeysManager::doOpenConnectionEditor()
{
    executeCommand("nm-connection-editor","");
}

void MediaKeysManager::doOpenUkuiSearchAction()
{
    executeCommand("ukui-search", " -s");
}

void MediaKeysManager::doOpenKdsAction()
{
    executeCommand("kydisplayswitch", "");
}

void MediaKeysManager::doUrlAction(const QString scheme)
{
    GError *error = NULL;
    GAppInfo *appInfo;

    appInfo = g_app_info_get_default_for_uri_scheme (scheme.toLatin1().data());

    if (appInfo != NULL) {
       if (!g_app_info_launch (appInfo, NULL, NULL, &error)) {
            syslog(LOG_DEBUG,"Could not launch '%s': %s",
                    g_app_info_get_commandline (appInfo),
                    error->message);
            g_object_unref (appInfo);
            g_error_free (error);
        }
    }else
        syslog(LOG_DEBUG,"Could not find default application for '%s' scheme",
                   scheme.toLatin1().data());
}

/**
 * @brief MediaKeysManager::doMultiMediaPlayerAction
 *        for detailed purposes,refer to the definition of MediaPlayerKeyPressed()
 *        有关该函数的详细用途，请参考MediaPlayerKeyPressed()的声明
 * @param operation
 *        @operation can take the following values: Play、Pause、Stop...
 */
void MediaKeysManager::doMultiMediaPlayerAction(const QString operation)
{
    if(!mediaPlayers.isEmpty())
        Q_EMIT MediaPlayerKeyPressed(mediaPlayers.first()->application,operation);
}


/**
 * @brief MediaKeysManager::GrabMediaPlayerKeys
 *        this is a dbus method,it will be called follow org.ukui.SettingsDaemon.MediaKeys in mpris plugin
 *        这是一个dbus method,它将会跟随org.ukui.SettingsDaemon.MediaKeys这个dbus在mpris插件中调用
 * @param app
 *        app=="UsdMpris" is true according to the open source.
 *        按照开源的写法，这个变量的值为"UsdMpris"
 * @param time
 */
void MediaKeysManager::GrabMediaPlayerKeys(QString app)
{
    QTime currentTime;
    uint curTime = 0;       //current time(s) 当前时间(秒)
    bool containApp;


    currentTime = QTime::currentTime();
    curTime = 60*currentTime.minute() + currentTime.second() + currentTime.msec()/1000;

    //whether @app is inclued in @mediaPlayers  @mediaPlayers中是否包含@app
    containApp = findMediaPlayerByApplication(app);

    if(true == containApp)
        removeMediaPlayerByApplication(app,curTime);

    syslog(LOG_DEBUG,"org.ukui.SettingsDaemon.MediaKeys registering %s at %u",app.toLatin1().data(),curTime);
    MediaPlayer* newPlayer = new MediaPlayer;
    newPlayer->application = app;
    newPlayer->time = curTime;
    mediaPlayers.insert(findMediaPlayerByTime(newPlayer),newPlayer);

}

/**
 * @brief MediaKeysManager::ReleaseMediaPlayerKeys
 * @param app
 */
void MediaKeysManager::ReleaseMediaPlayerKeys(QString app)
{
    bool containApp;
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    containApp = findMediaPlayerByApplication(app);

    if(true == containApp){
        for(; item != end; ++item){
            tmp = *item;
            //find @player successfully 在链表中成功找到该元素
            if(tmp->application == app){
                tmp->application.clear();
                delete tmp;
                mediaPlayers.removeOne(tmp);
                break;
            }
        }
    }
}

/**
 * @brief MediaKeysManager::findMediaPlayerByApplication
 * @param app   app=="UsdMpris" is true at general 一般情况下app=="UsdMpris"
 * @param player    if@app can be founded in @mediaPlayers, @player recored it's position
 *                  如果@app 存在于@mediaPlayers内，则@player 记录它的位置
 * @return return true if @app can be founded in @mediaPlayers
 */
bool MediaKeysManager::findMediaPlayerByApplication(const QString& app)
{
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    for(; item != end; ++item){
        tmp = *item;
        //find @player successfully 在链表中成功找到该元素
        if(tmp->application == app){
            return true;
        }
    }

    //can not find @player at QList<MediaPlayer*> 查找失败
    return false;
}

/**
 * @brief MediaKeysManager::findMediaPlayerByTime determine insert location at the @mediaPlayers
 *        确定在@mediaPlayers中的插入位置
 * @param player
 * @return  return index.    返回下标
 */
uint MediaKeysManager::findMediaPlayerByTime(MediaPlayer* player)
{
    if(mediaPlayers.isEmpty())
        return 0;
    return player->time < mediaPlayers.first()->time;
}

void MediaKeysManager::removeMediaPlayerByApplication(const QString& app,uint currentTime)
{
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    for(; item != end; ++item){
        tmp = *item;
        //find @player successfully 在链表中成功找到该元素
        if(tmp->application == app && tmp->time < currentTime){
            tmp->application.clear();
            delete tmp;
            mediaPlayers.removeOne(tmp);
            break;
        }
    }
}
