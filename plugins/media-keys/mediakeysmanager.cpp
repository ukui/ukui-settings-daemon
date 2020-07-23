#include <QTime>
#include "mediakeysmanager.h"
#include "eggaccelerators.h"

MediaKeysManager* MediaKeysManager::mManager = nullptr;

const int VOLUMESTEP = 6;
#define midValue(x,low,high) (((x) > (high)) ? (high): (((x) < (low)) ? (low) : (x)))

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
    QList<GdkScreen*>::iterator l,begin,end;

    syslog(LOG_DEBUG,"Starting mediakeys manager!");

    //mTimer = new QTimer();
    mSettings = new QGSettings("org.ukui.SettingsDaemon.plugins.media-keys");
    mScreenList = new QList<GdkScreen*>();
    mVolumeWindow = new VolumeWindow();
    mDeviceWindow = new DeviceWindow();
    mExecCmd = new QProcess();

    mVolumeWindow->initWindowInfo();
    mDeviceWindow->initWindowInfo();
    mate_mixer_init();

    if(mate_mixer_is_initialized()){
        mContext = mate_mixer_context_new();
        g_signal_connect(mContext,"notify::state",G_CALLBACK(onContextStateNotify),NULL);
        g_signal_connect(mContext,"notify::default-output-stream",G_CALLBACK(onContextDefaultOutputNotify),NULL);
        g_signal_connect(mContext,"notify::removed",G_CALLBACK(onContextStreamRemoved),NULL);

        mate_mixer_context_open(mContext);
    }

    initScreens();
    initKbd();

    l = begin = mScreenList->begin();
    end = mScreenList->end();
    for(; l != end; ++l){
        syslog(LOG_DEBUG,"adding key filter for screen: %d",gdk_screen_get_number(*l));
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
    delete mExecCmd;
    mExecCmd = nullptr;
    delete mVolumeWindow;
    mVolumeWindow = nullptr;
    delete mDeviceWindow;
    mDeviceWindow = nullptr;

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
    gdk_error_trap_push();
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
        gdk_flush();

    gdk_error_trap_pop_ignored();

    g_clear_object(&mStream);
    g_clear_object(&mControl);
    g_clear_object(&mContext);
}

void MediaKeysManager::initScreens()
{
    GdkDisplay *display;
    GdkScreen  *screen;
    int i,screenNums;

    display = gdk_display_get_default();
    screenNums = gdk_display_get_n_screens(display);

    for(i = 0; i < screenNums; ++i){
        screen = gdk_display_get_screen(display,i);
        if(NULL == screen)
            continue;
        mScreenList->append(screen);
    }
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

void MediaKeysManager::onContextStateNotify(MateMixerContext* context,GParamSpec* pspec,void* data)
{
    updateDefaultOutput();
}

void MediaKeysManager::onContextDefaultOutputNotify(MateMixerContext* context,GParamSpec* pspec,void* data)
{
    updateDefaultOutput();
}

void MediaKeysManager::onContextStreamRemoved(MateMixerContext* context,char* name,void* data)
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

void MediaKeysManager::updateDefaultOutput()
{
   MateMixerStream        *stream;
   MateMixerStreamControl *control = NULL;

   stream = mate_mixer_context_get_default_output_stream (mManager->mContext);
   if (stream != NULL)
           control = mate_mixer_stream_get_default_control (stream);

   if (stream == mManager->mStream)
           return;

   g_clear_object (&mManager->mStream);
   g_clear_object (&mManager->mControl);

   if (control != NULL) {
           MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (control);

           /* Do not use the stream if it is not possible to mute it or
            * change the volume */
           if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
               !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                   return;

           mManager->mStream = stream;
           mManager->mControl = control;
           syslog (LOG_DEBUG,"Default output stream updated to %s",
                    mate_mixer_stream_get_name (stream));
   } else
           syslog (LOG_DEBUG,"Default output stream unset");
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
        doScreensaverAction();
        break;
    case SETTINGS_KEY:
        doSettingsAction();
        break;
    case FILE_MANAGER_KEY:
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
        doOpenTerminalAction();
        break;
    case SCREENSHOT_KEY:
        doScreenshotAction("");
        break;
    case AREA_SCREENSHOT_KEY:
        doScreenshotAction(" -a");
        break;
    case WINDOW_SCREENSHOT_KEY:
        doScreenshotAction(" -w");
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

    gdk_error_trap_push();
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
        gdk_flush();
    if(gdk_error_trap_pop())
        syslog(LOG_WARNING,"Grab failed for some keys,another application may already have access the them.");
}

void MediaKeysManager::updateKbdCallback(const QString &key)
{
    int i;
    bool needFlush = true;

    if(key.isNull())
        return;

    gdk_error_trap_push ();

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
                exit(1);
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
        gdk_flush ();
    if (gdk_error_trap_pop ())
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

void MediaKeysManager::doSoundAction(int keyType)
{
    bool muted,mutedLast,soundChanged;  //是否静音，上一次值记录，是否改变
    uint volume,volumeMin,volumeMax;    //当前音量值，最小音量值，最大音量值
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
        muted = !muted;
        break;
    case VOLUME_DOWN_KEY:
        if(volume <= (volumeMin + volumeStep)){
            volume = volumeMin;
            muted = true;
        }else{
            volume -= volumeStep;
            muted = false;
        }
        break;
    case VOLUME_UP_KEY:
        if(muted){
            muted = false;
            if(volume <= (volumeMin + volumeStep))
                volume = volumeMin + volumeStep;
        }else
            volume = midValue(volume + volumeStep, volumeMin, volumeMax);
        break;
    }

    if(muted != mutedLast){
        if(mate_mixer_stream_control_set_mute(mControl,muted))
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
    QString cmd = command;
    processAbstractPath(cmd);
    if(!cmd.isEmpty())
        mExecCmd->start(cmd + paramter);
    else
        syslog(LOG_DEBUG,"%s cannot found at system path!",command.toLatin1().data());
}

void MediaKeysManager::doShutdownAction()
{
    executeCommand("ukui-session-tools"," --shutdown");
}

void MediaKeysManager::doLogoutAction()
{
    executeCommand("ukui-session-tools"," --logout");
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
    tool3 = "gnome-calculator";

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
    executeCommand("mate-screenshot",pramater);
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
