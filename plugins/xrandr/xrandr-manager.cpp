/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>
 * Copyright 2016 by Sebastian Kügler <sebas@kde.org>
 * Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
 *                    Work sponsored by the LiMux project of
 *                    the city of Munich.
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
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QX11Info>
#include "xrandr-manager.h"

#include <QOrientationReading>
#include <memory>

#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/log.h>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/configmonitor.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>

#include "usd_base_class.h"
extern "C"{
#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <xorg/xserver-properties.h>
#include <gudev/gudev.h>
#include "clib-syslog.h"
}

#define SETTINGS_XRANDR_SCHEMAS     "org.ukui.SettingsDaemon.plugins.xrandr"
#define XRANDR_ROTATION_KEY         "xrandr-rotations"
#define XRANDR_PC_ROTATION_KEY0     "xrandr-pc-rotation0"
#define XRANDR_PC_ROTATION_KEY1     "xrandr-pc-rotation1"
#define XRANDR_PRI_KEY              "primary"
#define XSETTINGS_SCHEMA            "org.ukui.SettingsDaemon.plugins.xsettings"
#define XSETTINGS_KEY_SCALING       "scaling-factor"
#define TABLET_MODE_KEY         "tablet-mode"

#define MAX_SIZE_MATCH_DIFF         0.05

#define DBUS_NAME  "org.ukui.SettingsDaemon"
#define DBUS_PATH  "/org/ukui/SettingsDaemon/wayland"
#define DBUS_INTER "org.ukui.SettingsDaemon.wayland"

#define LOGIN_DBUS_NAME     "org.freedesktop.login1"
#define LOGIN_DBUS_PATH     "/org/freedesktop/login1"
#define LOGIN_DBUS_INTER    "org.freedesktop.login1.Manager"

typedef struct
{
    unsigned char *input_node;
    XIDeviceInfo dev_info;

}TsInfo;

XrandrManager::XrandrManager()
{
    mSaveTimer = nullptr;
    time = new QTimer(this);
    mXrandrSetting = new QGSettings(SETTINGS_XRANDR_SCHEMAS);
    mChangeCompressor = new QTimer(this);
    mXsettings = new QGSettings(XSETTINGS_SCHEMA);
    mScale = mXsettings->get(XSETTINGS_KEY_SCALING).toDouble();


    mDbus = new xrandrDbus();
    new WaylandAdaptor(mDbus);
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if(sessionBus.registerService(DBUS_NAME)){
        sessionBus.registerObject(DBUS_PATH,
                                  mDbus,
                                  QDBusConnection::ExportAllContents);
    }

    mLoginInter = new QDBusInterface(LOGIN_DBUS_NAME,
                                     LOGIN_DBUS_PATH,
                                     LOGIN_DBUS_INTER,
                                     QDBusConnection::systemBus());

    if (false == UsdBaseClass::isWayland()) {
        KScreen::Log::instance();
        QMetaObject::invokeMethod(this, "getInitialConfig", Qt::QueuedConnection);
        connect(mLoginInter, SIGNAL(PrepareForSleep(bool)),this,
                SLOT(mPrepareForSleep(bool)));

        mApplyConfigTimer = new QTimer(this);
        connect(mApplyConfigTimer, &QTimer::timeout, this, &XrandrManager::applyConfigTimerHandle);
    }
}

void XrandrManager::getInitialConfig()
{
    connect(new KScreen::GetConfigOperation, &KScreen::GetConfigOperation::finished,
            this, [this](KScreen::ConfigOperation* op) {

        if (op->hasError()) {
            USD_LOG(LOG_DEBUG, "Error getting initial configuration %s" ,op->errorString().toLatin1().data());
            return;
        }

        USD_LOG(LOG_DEBUG,"usd xrandr init over...");

        if (mMonitoredConfig) {
            if(mMonitoredConfig->data()) {
                KScreen::ConfigMonitor::instance()->removeConfig(mMonitoredConfig->data());
                for (const KScreen::OutputPtr &output : mMonitoredConfig->data()->outputs()) {
                    output->disconnect(this);
                }
                mMonitoredConfig->data()->disconnect(this);
                USD_LOG(LOG_DEBUG,"usd xrandr init over...");
            }
            mMonitoredConfig = nullptr;
        }

        mMonitoredConfig = std::unique_ptr<xrandrConfig>(new xrandrConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config()));
        mMonitoredConfig->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);

        monitorsInit();
        applyConfig();

        USD_LOG(LOG_DEBUG,"usd xrandr init over...");
    });

    USD_LOG(LOG_DEBUG,"usd xrandr init over...");
}

XrandrManager::~XrandrManager()
{
    if(time)
        delete time;
    if(mXrandrSetting)
        delete mXrandrSetting;
    if(mLoginInter)
        delete mLoginInter;
    delete mXsettings;
}

bool XrandrManager::XrandrManagerStart()
{
    USD_LOG(LOG_DEBUG, "Xrandr Manager Start");
    connect(time, &QTimer::timeout, this,&XrandrManager::StartXrandrIdleCb);
    time->start();
    return true;
}

void XrandrManager::XrandrManagerStop()
{
    USD_LOG(LOG_DEBUG, "Xrandr Manager Stop");
}
#if 1
/*查找触摸屏设备ID*/
static bool
find_touchscreen_device(Display* display, XIDeviceInfo *dev)
{
    int i, j;
    if (dev->use != XISlavePointer)
        return false;
    if (!dev->enabled)
    {
        USD_LOG(LOG_DEBUG, "This device is disabled.");
        return false;
    }

    for (i = 0; i < dev->num_classes; i++)
    {
        if (dev->classes[i]->type == XITouchClass)
        {
            XITouchClassInfo *t = (XITouchClassInfo*)dev->classes[i];

            if (t->mode == XIDirectTouch)
            return true;
        }
    }
    return false;
}

/* Get device node for gudev
   node like "/dev/input/event6"
 */
unsigned char *
getDeviceNode (XIDeviceInfo devinfo)
{
    Atom  prop;
    Atom act_type;
    int  act_format;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    prop = XInternAtom(QX11Info::display(), XI_PROP_DEVICE_NODE, False);
    if (!prop)
        return NULL;

    if (XIGetProperty(QX11Info::display(), devinfo.deviceid, prop, 0, 1000, False,
                      AnyPropertyType, &act_type, &act_format, &nitems, &bytes_after, &data) == Success)
    {
        return data;
    }

    XFree(data);
    return NULL;
}

/* Get touchscreen info */
GList *
getTouchscreen(Display* display)
{
    gint n_devices;
    XIDeviceInfo *devs_info;
    int i;
    GList *ts_devs = NULL;
    Display *dpy = QX11Info::display();
    devs_info = XIQueryDevice(dpy, XIAllDevices, &n_devices);

    for (i = 0; i < n_devices; i ++)
    {
        if (find_touchscreen_device(dpy, &devs_info[i]))
        {
           unsigned char *node;
           TsInfo *ts_info = g_new(TsInfo, 1);
           node = getDeviceNode (devs_info[i]);

           if (node)
           {
               ts_info->input_node = node;
               ts_info->dev_info = devs_info[i];
               ts_devs = g_list_append (ts_devs, ts_info);
           }
        }
    }
    return ts_devs;
}

bool checkMatch(int output_width,  int output_height,
                double input_width, double input_height)
{
    double w_diff, h_diff;

    w_diff = ABS (1 - ((double) output_width / input_width));
    h_diff = ABS (1 - ((double) output_height / input_height));

    USD_LOG(LOG_ERR, "w_diff is %f, h_diff is %f\n", w_diff, h_diff);

    if (w_diff < MAX_SIZE_MATCH_DIFF && h_diff < MAX_SIZE_MATCH_DIFF) {
        return true;
    }
    else {
        return false;
    }
}

/* Here to run command xinput
   更新触摸屏触点位置
*/
void doAction (char *input_name, char *output_name)
{
    char buff[128]="";
    sprintf(buff, "xinput --map-to-output \"%s\" \"%s\"", input_name, output_name);

    USD_LOG(LOG_ERR, "buff is %s\n", buff);
    QProcess::execute(buff);
}

void SetTouchscreenCursorRotation()
{
    int     event_base, error_base, major, minor;
    int     o;
    Window  root;
    int     xscreen;
    XRRScreenResources *res;
    Display *dpy = QX11Info::display();

    GList *ts_devs = NULL;

    ts_devs = getTouchscreen (dpy);

    if (!g_list_length (ts_devs))
    {
        USD_LOG(LOG_ERR, "No touchscreen find...");
        return;
    }

    GList *l = NULL;

    if (!XRRQueryExtension (dpy, &event_base, &error_base) ||
        !XRRQueryVersion (dpy, &major, &minor))
    {
        USD_LOG(LOG_ERR, "RandR extension missing");
        return;
    }

    xscreen = DefaultScreen (dpy);
    root = RootWindow (dpy, xscreen);

    if (major >= 1 && minor >= 5)
    {
        res = XRRGetScreenResources (dpy, root);
        if (!res)
          return;

        for (o = 0; o < res->noutput; o++)
        {
            XRROutputInfo *output_info = XRRGetOutputInfo (dpy, res, res->outputs[o]);
            if (!output_info){
                USD_LOG(LOG_ERR, "Could not get output 0x%lx information", res->outputs[o]);
                continue;
            }

            if (output_info->connection == 0) {
                int output_mm_width = output_info->mm_width;
                int output_mm_height = output_info->mm_height;

                for ( l = ts_devs; l; l = l->next)
                {
                    TsInfo *info = (TsInfo *)l -> data;
                    GUdevDevice *udev_device;
                    const char *udev_subsystems[] = {"input", NULL};

                    double width, height;
                    GUdevClient *udev_client = g_udev_client_new (udev_subsystems);
                    udev_device = g_udev_client_query_by_device_file (udev_client,
                                                                      (const gchar *)info->input_node);

                    if (udev_device && g_udev_device_has_property (udev_device,
                                                                   "ID_INPUT_WIDTH_MM"))
                    {
                        width = g_udev_device_get_property_as_double (udev_device,
                                                                    "ID_INPUT_WIDTH_MM");
                        height = g_udev_device_get_property_as_double (udev_device,
                                                                     "ID_INPUT_HEIGHT_MM");

                        if (checkMatch(output_mm_width, output_mm_height, width, height)){
                            doAction(info->dev_info.name, output_info->name);
                        }
                    }
                    g_clear_object (&udev_client);
                }
            }
        }
    }else {
        g_list_free(ts_devs);
        USD_LOG(LOG_ERR, "xrandr extension too low");
        return;
    }
    g_list_free(ts_devs);
}
#endif

void XrandrManager::orientationChangedProcess(Qt::ScreenOrientation orientation)
{
    USD_LOG(LOG_DEBUG,"ready map touch device");
    SetTouchscreenCursorRotation();
     USD_LOG(LOG_DEBUG,"map touch device over");
}

/*监听旋转键值回调 并设置旋转角度*/
void XrandrManager::RotationChangedEvent(QString key)
{
    int angle;

    if(key != XRANDR_ROTATION_KEY || key != XRANDR_PC_ROTATION_KEY0){
        return;
    }

    angle = mXrandrSetting->getEnum(XRANDR_ROTATION_KEY);
    USD_LOG(LOG_DEBUG, "angle = %d", angle);

    const KScreen::OutputList outputs = mMonitoredConfig->data()->outputs();
    for(auto output : outputs){
        if (!output->isConnected() || !output->isEnabled() || !output->currentMode()) {
            continue;
        }
        output->setRotation(static_cast<KScreen::Output::Rotation>(angle));
        USD_LOG(LOG_DEBUG, "output rotation is %d, screen name is %s", output->rotation(), output->name().toLatin1().data());
    }
    USD_LOG(LOG_DEBUG,".");
    doApplyConfig(mMonitoredConfig->data());
}

void XrandrManager::applyIdealConfig()
{
    const bool showOsd = mMonitoredConfig->data()->connectedOutputs().count() > 1 && !mStartingUp;
    //qDebug()<<"show Sod  is "<<showOsd;
}

void XrandrManager::applyConfigTimerHandle()
{
    switch (mScreenCanBeApply) {
    case XrandrManager::coolDownStart:

        USD_LOG(LOG_DEBUG,"[coolDown]start run and enter coolDowning");
//        refreshConfig();
        setMonitorForChanges(false);
        applyConfig();
        primaryScreenChange();
        mSaveTimer->start();


        mApplyConfigTimer->start(500);
        mScreenCanBeApply = XrandrManager::coolDowning;
        break;
    case XrandrManager::coolDowning:

        USD_LOG(LOG_DEBUG,"[coolDown]cool down ok,,");

        mApplyConfigTimer->stop();
        mScreenCanBeApply = XrandrManager::coolDownStart;
        break;
    case XrandrManager::coolDownOverAndRun:
        mSaveTimer->start();
        setMonitorForChanges(false);
        mScreenCanBeApply = XrandrManager::coolDowning;
        USD_LOG(LOG_DEBUG,"[coolDown]cool down ok and run a new config");
        return;
    default:
        USD_LOG(LOG_ERR,"[coolDown]plz check screen apply status");
        break;
    }
}

void XrandrManager::doApplyConfig(const KScreen::ConfigPtr& config)
{
    USD_LOG(LOG_DEBUG,  "Do set and apply specific config");
    auto configWrapper = std::unique_ptr<xrandrConfig>(new xrandrConfig(config));
    configWrapper->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);

    doApplyConfig(std::move(configWrapper));
}

void XrandrManager::doApplyConfig(std::unique_ptr<xrandrConfig> config)
{
   mMonitoredConfig = std::move(config);

   refreshConfig();
   primaryScreenChange();

}

void XrandrManager::refreshConfig()
{
    setMonitorForChanges(false);
    USD_LOG(LOG_DEBUG,"refresh Config...");

    mConfigDirty = false;
    KScreen::ConfigMonitor::instance()->addConfig(mMonitoredConfig->data());

    connect(new KScreen::SetConfigOperation(mMonitoredConfig->data()),
            &KScreen::SetConfigOperation::finished,
            this, [this]() {

        USD_LOG(LOG_DEBUG,"setMonitorForChanges to true %d",mConfigDirty);
        setMonitorForChanges(true);

    });
}

void XrandrManager::printScreenApplyFailReason(KScreen::ConfigPtr KsData)
{
    int enabledOutputsCount = 0;

    Q_FOREACH(const KScreen::OutputPtr &output,KsData->outputs()) {
        if (!output->isEnabled()) {
            USD_LOG(LOG_DEBUG,"%s is disable",output->name().toLatin1().data());
            continue;
        }

        enabledOutputsCount++;
    }

    if (enabledOutputsCount >  KsData->screen()->maxActiveOutputsCount()) {
        USD_LOG(LOG_ERR,"config config be enabled..cuz maxActiveOutputsCount[%d] :[%d]",
                KsData->screen()->maxActiveOutputsCount(),enabledOutputsCount);
    } else {
        USD_LOG(LOG_DEBUG,"config config be enabled..cuz maxActiveOutputsCount[%d] :[%d]",
                KsData->screen()->maxActiveOutputsCount(),enabledOutputsCount);
    }

    USD_LOG(LOG_ERR,"had fail....");
}

void XrandrManager::saveCurrentConfig()
{
    USD_LOG(LOG_DEBUG, "Saving current config to file");

    // We assume the config is valid, since it's what we got, but we are interested
    // in the "at least one enabled screen" check

    if (mMonitoredConfig->canBeApplied()) {

        mMonitoredConfig->setPriName(mDbus->priScreenName());

        if (false == mMonitoredConfig->fileExists()) {
            USD_LOG(LOG_DEBUG,"%s is't exists creat it...",mMonitoredConfig->filePath().toLatin1().data());
            mMonitoredConfig->writeFile(mAddScreen);
            mAddScreen = false;
            mMonitoredConfig->log();
        } else {
            USD_LOG(LOG_DEBUG,"%s exists;",mMonitoredConfig->filePath().toLatin1().data());
        }
    } else {

        printScreenApplyFailReason(mMonitoredConfig->data());
        mMonitoredConfig->log();
    }
}

bool XrandrManager::pretreatScreenInfo()
{
    bool changed = false;
    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {

        USD_LOG_SHOW_OUTPUT(output);
        if (1 == mMonitoredConfig->data()->outputs().count() &&
                false == output->isEnabled() &&
                false == mMonitoredConfig->fileExists()
                )
        {
            USD_LOG_SHOW_OUTPUT(output);
            const QPoint pos(0,0);
            output->setPos(std::move(pos));
            changed = true;

            output->setEnabled(true);
            USD_LOG(LOG_DEBUG,  "enable %s screen...%dx%d at (%dx%d)",output->name().toLatin1().data(), output->size().width(), output->size().height(),
                                output->geometry().x(),output->geometry().y());
            primaryScreenChange();
            USD_LOG_SHOW_OUTPUT(output);

        }

        if (output->isConnected() && output->isEnabled() && (output->currentMode().isNull() || (output->followPreferredMode() && output->currentModeId() != output->preferredModeId()))) {
            output->setCurrentModeId(output->preferredModeId());
            changed = true;
        }
    }

    return changed;
}

void XrandrManager::configChanged()
{
    USD_LOG(LOG_DEBUG,  "Change detected");
    // Modes may have changed, fix-up current mode id
    bool changed;

    mApplyConfigTimer->stop();

    changed = pretreatScreenInfo();

    if (changed) {
        USD_LOG(LOG_DEBUG,"refresh config!!!");
        refreshConfig();
    }

    // Reset timer, delay the writeback
    if (!mSaveTimer) {
        mSaveTimer = new QTimer(this);
        mSaveTimer->setInterval(300);
        mSaveTimer->setSingleShot(true);
        connect(mSaveTimer, &QTimer::timeout, this, &XrandrManager::saveCurrentConfig);
    }
     mSaveTimer->start();
     setMonitorForChanges(false);

}

void XrandrManager::setMonitorForChanges(bool enabled)
{
    if (mMonitoring == enabled) {
        return;
    }

    USD_LOG(LOG_DEBUG, "Monitor for changes: %d", enabled);
    mMonitoring = enabled;
    if (mMonitoring) {
        connect(KScreen::ConfigMonitor::instance(), &KScreen::ConfigMonitor::configurationChanged,
                this, &XrandrManager::configChanged, Qt::UniqueConnection);
    } else {
        disconnect(KScreen::ConfigMonitor::instance(), &KScreen::ConfigMonitor::configurationChanged,
                   this, &XrandrManager::configChanged);
    }
}

void XrandrManager::applyKnownConfig(bool state)
{
    std::unique_ptr<xrandrConfig> readInConfig = mMonitoredConfig->readFile(state);

    if (!readInConfig) {
        USD_LOG(LOG_DEBUG,"read %s config fail...",state? "sleep":"normal");
    }

    if (readInConfig) {
        USD_LOG(LOG_DEBUG,".");
        doApplyConfig(std::move(readInConfig));
    } else {
        //qDebug() << "Loading failed, falling back to the ideal config" << mMonitoredConfig->id();
        applyIdealConfig();
    }
}

void XrandrManager::init_primary_screens (KScreen::ConfigPtr Config)
{
    KScreen::OutputList kscreenOutputs = Config->outputs();
    for (auto output : kscreenOutputs)
    {
        int x = output->geometry().x();
        if(x == 0)
            output->setPrimary(true);
    }
    USD_LOG(LOG_DEBUG,".");
    doApplyConfig(Config);
}

void XrandrManager::changeScreenPosition()
{
    int k = 0;
    int posX = 0;

    if(mMonitoredConfig->data()->outputs().count() < 2) {
        return;
    }


    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
        if (k == 0) {
            posX = output->size().width();
        } else {
            const QPoint pos(posX,0);
            output->setPos(pos);
            posX += output->size().width();
        }

        USD_LOG_SHOW_OUTPUT(output);
        k++;
    }
}

void XrandrManager::applyConfig()
{

    if (!mMonitoredConfig){
        USD_LOG(LOG_DEBUG, "mMonitoredConfig  is nullptr");
        return;
    }

    if(mSleepState){

        applyKnownConfig(false);//休眠模式与普通模式使用同一个配置文件
        mSleepState = false;
    }
    else if (mMonitoredConfig->fileExists()) {
        USD_LOG(LOG_DEBUG,"apply known config:%s", mMonitoredConfig->filePath().toLatin1().data());
        applyKnownConfig(false);
    }
    else {
        changeScreenPosition();
        init_primary_screens(mMonitoredConfig->data());
        saveCurrentConfig();
        USD_LOG(LOG_DEBUG,"apply unknown config");
    }

}

void XrandrManager::outputConnectedChanged()
{
    KScreen::Output *output = qobject_cast<KScreen::Output*>(sender());

    if (output->isConnected()) {
        Q_EMIT outputConnected(output->name());

        if (!mMonitoredConfig->fileExists()) {
            Q_EMIT unknownOutputConnected(output->name());
        }
    }
}

void XrandrManager::outputAdded(const KScreen::OutputPtr &output)
{

    USD_LOG_SHOW_OUTPUT(output);
    if (false == mMonitoredConfig->data()->outputs().contains(output->id())) {
        mMonitoredConfig->data()->addOutput(output->clone());
        USD_LOG(LOG_DEBUG,"[%s] miss in OutPuts so usd add it", output->name().toLatin1().data());
    }

    if (output->isConnected()) {
        mAddScreen = true;
        mChangeCompressor->start();
    }
}

void XrandrManager::outputRemoved(int outputId)
{

    if (true == mMonitoredConfig->data()->outputs().contains(outputId)) {
           mMonitoredConfig->data()->removeOutput(outputId);
           USD_LOG(LOG_DEBUG,"d had't removed...%doutputId", mMonitoredConfig->data()->outputs().count() );
    }

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
        USD_LOG_SHOW_OUTPUT(output);

        if (1 == mMonitoredConfig->data()->outputs().count() && false == output->isEnabled()) {
            if (false == mMonitoredConfig->fileExists())
            {
                USD_LOG(LOG_DEBUG,"emit a config changed cuz disable screen'config isn't exists then can't emit changed");
                configChanged();//拔掉已关闭的屏幕并且文件不存在时，不会产生configchanged信号，需要手动触发。
                return;
            }
        }
    }

    applyConfig();
}

void XrandrManager::primaryOutputChanged(const KScreen::OutputPtr &output) {
    Q_ASSERT(mConfig);

    USD_LOG(LOG_DEBUG,"primary output changed %d",output.isNull());
    int index = output.isNull() ? 0 : output->id();

    if(index != 0) {
        QRect geometry = output->geometry();
        //callMethod(geometry, output->name());
    }
}

void XrandrManager::primaryScreenChange()
{
    QRect geometry;
    QString name;

    if (mMonitoredConfig->data()->primaryOutput().isNull()) {
        USD_LOG(LOG_DEBUG, "No primary screen output was found");
        const KScreen::OutputList outputs = mMonitoredConfig->data()->outputs();
        for(auto output : outputs) {
            if (!output->isConnected() || !output->isEnabled() || !output->currentMode()) {
                continue;
            }
            geometry = output->geometry();
            name = output->name();
            break;
        }
    }
    else {
        if (false == mMonitoredConfig->data()->primaryOutput()->isEnabled() && mMonitoredConfig->data()->outputs().count() > 1) {
            USD_LOG(LOG_DEBUG, "%s is diable can't reset output's pos..",
                    mMonitoredConfig->data()->primaryOutput()->name().toLatin1().data());
            return;
        }

        geometry = mMonitoredConfig->data()->primaryOutput()->geometry();
        name     = mMonitoredConfig->data()->primaryOutput()->name();
    }

//    callMethod(geometry, name);
}

void XrandrManager::callMethod(QRect geometry, QString name)
{
    QDBusMessage message =
            QDBusMessage::createMethodCall(DBUS_NAME,
                                           DBUS_PATH,
                                           DBUS_INTER,
                                           "priScreenChanged");

    int x = ceil(geometry.x()/mScale);
    int y = ceil(geometry.y()/mScale);
    int w = ceil(geometry.width()/mScale);
    int h = ceil(geometry.height()/mScale);
    message << x<< y << w << h << name;
    USD_LOG(LOG_DEBUG, "x= %d, y=%d, width = %d, height = %d", x, y, w, h);
    QDBusMessage response = QDBusConnection::sessionBus().call(message);
    if (response.type() != QDBusMessage::ReplyMessage){
        USD_LOG(LOG_DEBUG, "priScreenChanged called failed");
    }
}

void XrandrManager::monitorsInit()
{
    mChangeCompressor->setInterval(10);
    mChangeCompressor->setSingleShot(true);
    connect(mChangeCompressor, &QTimer::timeout, this, &XrandrManager::applyConfig);
    if (mConfig) {
        KScreen::ConfigMonitor::instance()->removeConfig(mConfig);
        for (const KScreen::OutputPtr &output : mConfig->outputs()) {
            output->disconnect(this);
        }
        mConfig->disconnect(this);
    }
    mConfig = mMonitoredConfig->data();

    KScreen::ConfigMonitor::instance()->addConfig(mConfig);
    connect(mConfig.data(), &KScreen::Config::outputAdded,
            this, &XrandrManager::outputAdded), Qt::UniqueConnection;
    connect(mConfig.data(), &KScreen::Config::outputRemoved,
            this, &XrandrManager::outputRemoved,
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    connect(mConfig.data(), &KScreen::Config::primaryOutputChanged,
            this, &XrandrManager::primaryOutputChanged);

}

void XrandrManager::mPrepareForSleep(bool state)
{

#if 0
    if (!state){
        g_usleep(1000*1000);
        mSleepState = true;
        mChangeCompressor->start();
    } else {
        mMonitoredConfig->setPriName(mDbus->priScreenName());
        setMonitorForChanges(false);
        QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) %
                                                            QStringLiteral("/sleep-state/") %
                                                           QStringLiteral("" /*"configs/"*/);
        QString hash = mMonitoredConfig->data()->connectedOutputsHash();
        QDir().mkpath(dir);
        mMonitoredConfig->writeFile(dir % hash, true);
    }
#endif

}

void XrandrManager::calingPeonyProcess()
{
    QString str = "/usr/bin/peony-qt-desktop -b";
    QProcess::startDetached(str);
}

/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */

void XrandrManager::StartXrandrIdleCb()
{

    time->stop();
    mScreen = QApplication::primaryScreen();

    if(!mScreen) {
        mScreen = QApplication::screens().at(0);
    }

    connect(mXrandrSetting, &QGSettings::changed, this, &XrandrManager::RotationChangedEvent);
    connect(mScreen, &QScreen::orientationChanged, this, &XrandrManager::orientationChangedProcess);



}
