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


#include <QDBusMessage>

extern "C"{
#include <glib.h>
//#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <xorg/xserver-properties.h>
#include <gudev/gudev.h>
#include "clib-syslog.h"
}

#define SETTINGS_XRANDR_SCHEMAS     "org.ukui.SettingsDaemon.plugins.xrandr"
#define XRANDR_ROTATION_KEY         "xrandr-rotations"
#define XRANDR_PRI_KEY              "primary"
#define XSETTINGS_SCHEMA            "org.ukui.SettingsDaemon.plugins.xsettings"
#define XSETTINGS_KEY_SCALING       "scaling-factor"

#define MAX_SIZE_MATCH_DIFF         0.05


unsigned char *getDeviceNode (XIDeviceInfo devinfo);
typedef struct
{
    unsigned char *input_node;
    XIDeviceInfo dev_info;

}TsInfo;

XrandrManager::XrandrManager()
{
    QGSettings *mXsettings = new QGSettings(XSETTINGS_SCHEMA);
    mScale = mXsettings->get(XSETTINGS_KEY_SCALING).toDouble();

    KScreen::Log::instance();

    mDbus = new xrandrDbus(this);
    mXrandrSetting = new QGSettings(SETTINGS_XRANDR_SCHEMAS);

    new XrandrAdaptor(mDbus);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.registerService(DBUS_XRANDR_NAME)) {
        sessionBus.registerObject(DBUS_XRANDR_PATH,
                                  mDbus,
                                  QDBusConnection::ExportAllContents);
    }

    mAcitveTime = new QTimer(this);
    mKscreenInitTimer = new QTimer(this);

    {
        QMetaObject mo = XrandrManager::staticMetaObject;
        int index = mo.indexOfEnumerator("eScreenMode");

        metaEnum = QMetaEnum::fromType<UsdBaseClass::eScreenMode>();
    }

    m_DbusRotation = new QDBusInterface("com.kylin.statusmanager.interface","/","com.kylin.statusmanager.interface",QDBusConnection::sessionBus(),this);

    if (nullptr != m_DbusRotation) {
        if (m_DbusRotation->isValid()) {
            connect(m_DbusRotation, SIGNAL(rotations_change_signal(QString)),this,SLOT(RotationChangedEvent(QString)));
            // USD_LOG(LOG_DEBUG, "m_DbusRotation ok....");
        } else {
            USD_LOG(LOG_ERR, "m_DbusRotation fail...");
        }
    }

    t_DbusTableMode = new QDBusInterface("com.kylin.statusmanager.interface","/","com.kylin.statusmanager.interface",QDBusConnection::sessionBus(),this);

    if (t_DbusTableMode->isValid()) {
        connect(t_DbusTableMode, SIGNAL(mode_change_signal(bool)),this,SLOT(TabletSettingsChanged(bool)));
        //USD_LOG(LOG_DEBUG, "..");
    } else {
        USD_LOG(LOG_ERR, "m_DbusRotation");
    }
}

void XrandrManager::getInitialConfig()
{
    connect(new KScreen::GetConfigOperation, &KScreen::GetConfigOperation::finished,
            this, [this](KScreen::ConfigOperation* op) {
        mKscreenInitTimer->stop();
        if (op->hasError()) {
            USD_LOG(LOG_DEBUG,"Error getting initial configuration：%s",op->errorString().toLatin1().data());
            return;
        }
        if (mMonitoredConfig) {
            if (mMonitoredConfig->data()) {
                KScreen::ConfigMonitor::instance()->removeConfig(mMonitoredConfig->data());
                for (const KScreen::OutputPtr &output : mMonitoredConfig->data()->outputs()) {
                    output->disconnect(this);
                }
                mMonitoredConfig->data()->disconnect(this);
            }
            mMonitoredConfig = nullptr;
        }

        mMonitoredConfig = std::unique_ptr<xrandrConfig>(new xrandrConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config()));
        mMonitoredConfig->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);
        monitorsInit();

        mDbus->mScreenMode = discernScreenMode();

        mMonitoredConfig->setScreenMode(metaEnum.valueToKey(mDbus->mScreenMode));
        USD_LOG(LOG_DEBUG, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!mDbus mode:%s!!!!!!!!!!!!!!!!!!!!!!!1",metaEnum.key(mDbus->mScreenMode));
    });
    USD_LOG(LOG_DEBUG,"kscreen init ..");
}

XrandrManager::~XrandrManager()
{
    if (mAcitveTime) {
        delete mAcitveTime;
        mAcitveTime = nullptr;
    }
    if (mXrandrSetting) {
        delete mXrandrSetting;
        mXrandrSetting = nullptr;
    }
    if (mXsettings) {
        delete mXsettings;
        mXsettings = nullptr;
    }

    //  if(mLoginInter) {
    //     delete mLoginInter;
    //      mLoginInter = nullptr;
    //  }
}

bool XrandrManager::XrandrManagerStart()
{
    USD_LOG(LOG_DEBUG,"Xrandr Manager Start");
    connect(mAcitveTime, &QTimer::timeout, this,&XrandrManager::StartXrandrIdleCb);
    mAcitveTime->start();
    return true;
}

void XrandrManager::XrandrManagerStop()
{
     USD_LOG(LOG_DEBUG,"Xrandr Manager Stop");
}

/*查找触摸屏设备ID*/
static bool
find_touchscreen_device(Display* display, XIDeviceInfo *dev)
{
    int i, j;
    if (dev->use != XISlavePointer) {
        return false;
    }

    if (!dev->enabled) {
        USD_LOG(LOG_DEBUG,"%s device is disabled.",dev->name);
        return false;
    }

/*  //just for print device class
    for (i = 0; i < dev->num_classes; i++) {

        if (dev->classes[i]->type == XIButtonClass) {
            XIButtonClassInfo *t = (XIButtonClassInfo*)dev->classes[i];
            USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->type);
        } else if (dev->classes[i]->type == XIValuatorClass) {
            XIValuatorClassInfo *t = (XIValuatorClassInfo*)dev->classes[i];
            USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->mode);

        } else if (dev->classes[i]->type == XIScrollClass) {
            XIScrollClassInfo *t = (XIScrollClassInfo*)dev->classes[i];
            USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->type);

        }else if (dev->classes[i]->type == XITouchClass) {
            XITouchClassInfo *t = (XITouchClassInfo*)dev->classes[i];
            USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->mode);

        }
    }
*/

    QString devName = QString::fromUtf8(dev->name);
    if (devName.toUpper().contains("TOUCHPAD") || devName.toUpper().contains("PEN")) { //触摸板也需要映射
        return true;
    }

    for (i = 0; i < dev->num_classes; i++) {

        if (dev->classes[i]->type == XITouchClass)
        {
            XITouchClassInfo *t = (XITouchClassInfo*)dev->classes[i];
//            USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->mode);
            if (t->mode == XIDirectTouch) {
//                USD_LOG(LOG_DEBUG,"%s type:%d mode:%d", dev->name, dev->classes[i]->type, t->mode);
                return true;
            }
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


//        if (g_udev_device_has_property(udev_device, "ID_INPUT_WIDTH_MM") || deviceName.toUpper().contains("TOUCHPAD"))

        if (find_touchscreen_device(dpy, &devs_info[i])) {
            unsigned char *node;
            TsInfo *ts_info = g_new(TsInfo, 1);
            node = getDeviceNode (devs_info[i]);

            if (node) {
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



    if (w_diff < MAX_SIZE_MATCH_DIFF && h_diff < MAX_SIZE_MATCH_DIFF) {
//          printf("w_diff is %f, h_diff is %f\n", w_diff, h_diff);
//        USD_LOG_SHOW_PARAM2(output_width, output_height);
//        USD_LOG_SHOW_PARAM2F(input_width, input_height);
//        USD_LOG_SHOW_PARAM2F(w_diff, h_diff);
//        USD_LOG(LOG_DEBUG,"...right");
        return true;
    }
    else
        return false;
}

/* Here to run command xinput
   更新触摸屏触点位置
*/

void doAction (int input_name, char *output_name)
{
    char buff[128] = "";
    sprintf(buff, "xinput --map-to-output \"%d\" \"%s\"", input_name, output_name);
    USD_LOG(LOG_DEBUG,"map touch-screen [%s]\n", buff);
    QProcess::execute(buff);
}


/*
 *
 * 触摸设备映射方案：
 * 首先找出输出设备的尺寸，然后找到触摸设备的尺寸，最后如果尺寸一致，则为一一对应的关系，需要处理映射。
 *
 */
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
        fprintf(stdin, "No touchscreen find...\n");
        return;
    }

    GList *l = NULL;

    if (!XRRQueryExtension (dpy, &event_base, &error_base) ||
        !XRRQueryVersion (dpy, &major, &minor))
    {
        fprintf (stderr, "RandR extension missing\n");
        return;
    }

    xscreen = DefaultScreen (dpy);
    root = RootWindow (dpy, xscreen);

    if ( major >= 1 && minor >= 5) {
        res = XRRGetScreenResources (dpy, root);
        if (!res)
          return;

        for (o = 0; o < res->noutput; o++)
        {
            XRROutputInfo *output_info = XRRGetOutputInfo (dpy, res, res->outputs[o]);
            if (!output_info){
                fprintf (stderr, "could not get output 0x%lx information\n", res->outputs[o]);
                continue;
            }
            int output_mm_width = output_info->mm_width;
            int output_mm_height = output_info->mm_height;

//            USD_LOG_SHOW_PARAM2(output_mm_width,output_mm_height);
//            USD_LOG_SHOW_PARAM1(output_info->connection);

            if (output_info->connection == 0) {
                for ( l = ts_devs; l; l = l->next) {
                    TsInfo *info = (TsInfo *)l -> data;
                    double width, height;
                    QString deviceName = QString::fromLocal8Bit(info->dev_info.name);
                    const char *udev_subsystems[] = {"input", NULL};

                    GUdevDevice *udev_device;
                    GUdevClient *udev_client = g_udev_client_new (udev_subsystems);
                    udev_device = g_udev_client_query_by_device_file (udev_client,
                                                                      (const gchar *)info->input_node);

//                    USD_LOG(LOG_DEBUG,"%s(%d) %d had touch",info->dev_info.name,info->dev_info.deviceid,g_udev_device_has_property(udev_device,"ID_INPUT_WIDTH_MM"), g_udev_device_has_property(udev_device,"ID_INPUT_HEIGHT_MM"));

                    //sp1的触摸板不一定有此属性，所以根据名字进行适配by sjh 2021年10月20日11:23:58
                    if ((udev_device && g_udev_device_has_property (udev_device,
                                                                   "ID_INPUT_WIDTH_MM")) || deviceName.toUpper().contains("TOUCHPAD") || deviceName.toUpper().contains("PEN")) {
                        width = g_udev_device_get_property_as_double (udev_device,
                                                                    "ID_INPUT_WIDTH_MM");
                        height = g_udev_device_get_property_as_double (udev_device,
                                                                     "ID_INPUT_HEIGHT_MM");

//                        USD_LOG(LOG_DEBUG,"%s %fx%f screen:%dx%d",info->dev_info.name, width,height, output_mm_width, output_mm_height);

                        if (checkMatch(output_mm_width, output_mm_height, width, height) || deviceName.toUpper().contains("TOUCHPAD")) {//
                            doAction(info->dev_info.deviceid,output_info->name);
                        }
                    }
                    g_clear_object (&udev_client);
                }
            }
        }
    }else {
        g_list_free(ts_devs);
        fprintf(stderr, "xrandr extension too low\n");
        return;
    }
    g_list_free(ts_devs);
}

void XrandrManager::orientationChangedProcess(Qt::ScreenOrientation orientation)
{
    SetTouchscreenCursorRotation();
}

/*监听旋转键值回调 并设置旋转角度*/
void XrandrManager::RotationChangedEvent(const QString &rotation)
{
    int value;
    QString angle_Value = rotation;

    if (angle_Value == "normal") {
        value = 1;
    } else if (angle_Value == "left") {
        value = 2;
    } else if (angle_Value == "upside-down") {
        value = 4;
    } else if  (angle_Value == "right") {
        value = 8;
    } else {
        USD_LOG(LOG_DEBUG,"Find a error !!!");
    }

    const KScreen::OutputList outputs = mMonitoredConfig->data()->outputs();
    for(auto output : outputs){
        if (!output->isConnected() || !output->isEnabled() || !output->currentMode()) {
            continue;
        }
        output->setRotation(static_cast<KScreen::Output::Rotation>(value));
//        qDebug()<<output->rotation() <<output->name();
        USD_LOG(LOG_DEBUG,"set %s rotaion:%s", output->name().toLatin1().data(), rotation.toLatin1().data());
    }

    applyConfig();
}

void XrandrManager::applyIdealConfig()
{
    const bool showOsd = mMonitoredConfig->data()->connectedOutputs().count() > 1 && !mStartingUp;
    //qDebug()<<"show Sod  is "<<showOsd;
}

void XrandrManager::doApplyConfig(const KScreen::ConfigPtr& config)
{
    //qDebug() << "Do set and apply specific config";
    auto configWrapper = std::unique_ptr<xrandrConfig>(new xrandrConfig(config));
    configWrapper->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);

    doApplyConfig(std::move(configWrapper));
}

void XrandrManager::doApplyConfig(std::unique_ptr<xrandrConfig> config)
{
    mMonitoredConfig = std::move(config);
    //monitorsInit();
    refreshConfig();
    primaryScreenChange();
}

void XrandrManager::refreshConfig()
{

}

void XrandrManager::saveCurrentConfig()
{

}

void XrandrManager::configChanged()
{

}

void XrandrManager::setMonitorForChanges(bool enabled)
{
    if (mMonitoring == enabled) {
        return;
    }

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

}

void XrandrManager::init_primary_screens (KScreen::ConfigPtr Config)
{

}

void XrandrManager::applyConfig()
{
    connect(new KScreen::SetConfigOperation(mMonitoredConfig->data()),
            &KScreen::SetConfigOperation::finished,
            this, [this]() {
        USD_LOG(LOG_DEBUG,"set screen params ok. start map touch device!");
        SetTouchscreenCursorRotation();
        mMonitoredConfig->writeFile(true);//首次接入
        Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
            if (output->isConnected()) {
                USD_LOG_SHOW_OUTPUT(output);
            }
        }
    });
}

void XrandrManager::outputConnectedChanged()
{

}

void XrandrManager::outputAddedHandle(const KScreen::OutputPtr &output)
{
    USD_LOG(LOG_DEBUG,".");

}

void XrandrManager::outputRemoved(int outputId)
{
     USD_LOG(LOG_DEBUG,".");

}

void XrandrManager::primaryOutputChanged(const KScreen::OutputPtr &output)
{
    USD_LOG(LOG_DEBUG,".");
}

void XrandrManager::primaryScreenChange()
{
    USD_LOG(LOG_DEBUG,".");
}

void XrandrManager::callMethod(QRect geometry, QString name)
{
    Q_UNUSED(geometry);
    Q_UNUSED(name);
}


/*
 *
 *接入时没有配置文件的处理流程：
 *单屏：最优分辨率。
 *多屏幕：镜像模式。
 *
*/
void XrandrManager::outputConnectedWithoutConfigFile(KScreen::Output *newOutput, char outputCount)
{
    if (1 == outputCount) {//单屏接入时需要设置模式，主屏
        newOutput->setCurrentModeId(newOutput->preferredModeId());
        newOutput->setPrimary(true);
    } else {
        setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::cloneScreenMode));
    }

}

void XrandrManager::lightLastScreen()
{
    int enableCount = 0;
    int connectCount = 0;

    Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
        if (output->isConnected()){
            connectCount++;
        }
        if (output->isEnabled()){
            enableCount++;
        }
    }
    if (connectCount==1 && enableCount==0){
        Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
            if (output->isConnected()){
                output->setEnabled(true);
                break;
            }
        }
    }
}

uint8_t XrandrManager::getCurrentRotation()
{
    uint8_t ret;
    QDBusMessage message = QDBusMessage::createMethodCall(DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_PATH,
                                                          DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_GET_ROTATION);

    QDBusMessage response = QDBusConnection::sessionBus().call(message);

    if (response.type() == QDBusMessage::ReplyMessage) {
        if(response.arguments().isEmpty() == false) {
            QString value = response.arguments().takeFirst().toString();
            USD_LOG(LOG_DEBUG, "get mode :%s", value.toLatin1().data());

            if (value == "normal") {
                ret = 1;
            } else if (value == "left") {
                 ret = 2;
            } else if (value == "upside-down") {
                  ret = 4;
            } else if  (value == "right") {
                   ret = 8;
            } else {
                USD_LOG(LOG_DEBUG,"Find a error !!! value%s",value.toLatin1().data());
                return ret = 1;
            }
        }
    }

    return ret;
}

/*
 *
 * -1:无接口
 * 0:PC模式
 * 1：tablet模式
 *
*/
int8_t XrandrManager::getCurrentMode()
{
    QDBusMessage message = QDBusMessage::createMethodCall(DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_PATH,
                                                          DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_GET_MODE);

    QDBusMessage response = QDBusConnection::sessionBus().call(message);

    if (response.type() == QDBusMessage::ReplyMessage) {
        if(response.arguments().isEmpty() == false) {
            bool value = response.arguments().takeFirst().toBool();
            USD_LOG(LOG_DEBUG, "get mode :%d", value);
            return value;
        }
    }

    return -1;
}
void XrandrManager::outputChangedHandle(KScreen::Output *senderOutput)
{
    char outputConnectCount = 0;
    USD_LOG_SHOW_OUTPUT(senderOutput);

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
        if (output->name()==senderOutput->name() && output->hash()!=senderOutput->hash()) {
            USD_LOG(LOG_DEBUG,"reset output..");
            USD_LOG_SHOW_OUTPUT(output);
            senderOutput->setEnabled(true);

            mMonitoredConfig->data()->removeOutput(output->id());
            mMonitoredConfig->data()->addOutput(senderOutput->clone());
//            output->setEdid(senderOutput->edid()->clone());
            USD_LOG_SHOW_OUTPUT(output);
            break;
        }
    }

    Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
        USD_LOG_SHOW_OUTPUT(output);

        if (output->name()==senderOutput->name()) {//这里只设置connect，enbale由配置设置。
            output->setEnabled(senderOutput->isConnected());
             output->setConnected(senderOutput->isConnected());

            if (0 == output->modes().count()) {
                output->setModes(senderOutput->modes());
            }

           USD_LOG(LOG_DEBUG,"find and set it....%s",output->name().toLatin1().data());
       }

        if (output->isConnected()) {
            outputConnectCount++;
        }
    }

    if (UsdBaseClass::isTablet()) {

        int ret = getCurrentMode();
        USD_LOG(LOG_DEBUG,"intel edu is't need use config file");
        if (0 < ret) {
            //tablet模式
              setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::cloneScreenMode));
        } else {
            //PC模式
              setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::extendScreenMode));
        }
    } else {//非intel项目无此接口
         USD_LOG(LOG_DEBUG,"need to use config file");
        if (false == mMonitoredConfig->fileExists()) {
            USD_LOG(LOG_DEBUG,"config %s is't exists.connect:%d.", mMonitoredConfig->filePath().toLatin1().data(),outputConnectCount);
            if (senderOutput->isConnected()) {
                senderOutput->setEnabled(senderOutput->isConnected());
            }
            outputConnectedWithoutConfigFile(senderOutput, outputConnectCount);
            mMonitoredConfig->writeFile(true);//首次接入，

        } else {
            USD_LOG(LOG_DEBUG,"%s it...FILE:%s",senderOutput->isConnected()? "Enable":"Disable",mMonitoredConfig->filePath().toLatin1().data());
            if (outputConnectCount) {
                mMonitoredConfig = mMonitoredConfig->readFile(false);
            }
        }

        lightLastScreen();
        applyConfig();
    }
}

//处理来自控制面板的操作,保存配置
void XrandrManager::SaveConfigTimerHandle()
{
    mSaveConfigTimer->stop();


    mMonitoredConfig->setScreenMode(metaEnum.valueToKey(mDbus->mScreenMode));
    mMonitoredConfig->writeFile(true);

    USD_LOG(LOG_DEBUG,"start send signal....");
    mDbus->sendModeChangeSignal(discernScreenMode());
    mDbus->sendScreensParamChangeSignal(mMonitoredConfig->getScreensParam());
}

QString XrandrManager::getScreesParam()
{
    return mMonitoredConfig->getScreensParam();
}

void XrandrManager::monitorsInit()
{
    char connectedOutputCount = 0;
    if (mConfig) {
        KScreen::ConfigMonitor::instance()->removeConfig(mConfig);
        for (const KScreen::OutputPtr &output : mConfig->outputs()) {
            output->disconnect(this);
        }
        mConfig->disconnect(this);
    }

    mConfig = std::move(mMonitoredConfig->data());

    for (const KScreen::OutputPtr &output: mConfig->outputs()) {
        USD_LOG_SHOW_OUTPUT(output);

        if (output->isConnected()){
            connectedOutputCount++;
        }


        //支负责插拔恢复
        connect(output.data(), &KScreen::Output::outputChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG_SHOW_OUTPUT(senderOutput);
            outputChangedHandle(senderOutput);
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::isPrimaryChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG_SHOW_OUTPUT(senderOutput);
            USD_LOG(LOG_DEBUG,"PrimaryChanged:%s",senderOutput->name().toLatin1().data());

            Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
                if (output->name() == senderOutput->name()) {
                    USD_LOG_SHOW_OUTPUT(output);
                    output->setPrimary(senderOutput->isPrimary());
                    USD_LOG_SHOW_OUTPUT(output);
                    break;
                }
            }
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::posChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"posChanged:%s",senderOutput->name().toLatin1().data());

            Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
                if (output->name() == senderOutput->name()) {
                    USD_LOG_SHOW_OUTPUT(output);
                    output->setPos(senderOutput->pos());
                    USD_LOG_SHOW_OUTPUT(output);
                    break;
                }
            }
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::sizeChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"sizeChanged:%s",senderOutput->name().toLatin1().data());
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::clonesChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"clonesChanged:%s",senderOutput->name().toLatin1().data());
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::rotationChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"rotationChanged:%s",senderOutput->name().toLatin1().data());
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::currentModeIdChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"currentModeIdChanged:%s",senderOutput->name().toLatin1().data());

            Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
                if (output->name() == senderOutput->name()) {
                    output->setCurrentModeId(senderOutput->currentModeId());
                    break;
                }
            }

            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });

        connect(output.data(), &KScreen::Output::isEnabledChanged, this, [this](){
            KScreen::Output *senderOutput = static_cast<KScreen::Output*> (sender());
            USD_LOG(LOG_DEBUG,"isEnabledChanged:%s",senderOutput->name().toLatin1().data());
            USD_LOG_SHOW_OUTPUT(senderOutput);
            Q_FOREACH(const KScreen::OutputPtr &output,mMonitoredConfig->data()->outputs()) {
                if (output->name() == senderOutput->name()) {
                    USD_LOG_SHOW_OUTPUT(output);
                    output->setEnabled(senderOutput->isEnabled());
                    USD_LOG_SHOW_OUTPUT(output);
                    break;
                }
            }
            mSaveConfigTimer->start(SAVE_CONFIG_TIME);
        });
    }

    KScreen::ConfigMonitor::instance()->addConfig(mConfig);
    //connect(mConfig.data(), &KScreen::Config::outputAdded,
    //        this, &XrandrManager::outputAddedHandle);

    connect(mConfig.data(), SIGNAL(outputAdded(KScreen::OutputPtr)),
            this, SLOT(outputAddedHandle(KScreen::OutputPtr)));

    connect(mConfig.data(), &KScreen::Config::outputRemoved,
            this, &XrandrManager::outputRemoved,
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));

    connect(mConfig.data(), &KScreen::Config::primaryOutputChanged,
            this, &XrandrManager::primaryOutputChanged);

    if (mMonitoredConfig->fileExists()){
        USD_LOG(LOG_DEBUG,"read  config:%s.",mMonitoredConfig->filePath().toLatin1().data());
        mMonitoredConfig = mMonitoredConfig->readFile(false);
        applyConfig();
    } else {
        int foreachTimes = 0;
        USD_LOG(LOG_DEBUG,"creat a config:%s.",mMonitoredConfig->filePath().toLatin1().data());

        for (const KScreen::OutputPtr &output: mMonitoredConfig->data()->outputs()) {
            USD_LOG_SHOW_OUTPUT(output);
            if (1==connectedOutputCount){
                outputChangedHandle(output.data());
                break;
            }
            else {

                if (output->isConnected()){
                    foreachTimes++;
                }

                if (foreachTimes>1) {
                    USD_LOG_SHOW_OUTPUT(output);
                    outputChangedHandle(output.data());
                    break;
                }

            }
        }
        mMonitoredConfig->writeFile(false);
    }
}

bool XrandrManager::checkPrimaryScreenIsSetable()
{
    int connecedScreenCount = 0;

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()){
        if (output->isConnected()){
            connecedScreenCount++;
        }
    }

    if (connecedScreenCount < 2) {
        USD_LOG(LOG_DEBUG, "skip set command cuz ouputs count :%d",mMonitoredConfig->data()->outputs().count());
        return false;
    }

    if (nullptr == mMonitoredConfig->data()->primaryOutput()){
        USD_LOG(LOG_DEBUG,"can't find primary screen.");
        Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
            if (output->isConnected()){
                output->setPrimary(true);
                output->setEnabled(true);
                USD_LOG(LOG_DEBUG,"set %s as primary screen.",output->name().toLatin1().data());
                break;
            }
        }
    }
    return true;
}


bool XrandrManager::readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode eMode)
{

     if (UsdBaseClass::isTablet())
     {
         return false;
     }

    mMonitoredConfig->setScreenMode(metaEnum.valueToKey(eMode));

    if (mMonitoredConfig->fileScreenModeExists(metaEnum.valueToKey(eMode))) {

        mMonitoredConfig = mMonitoredConfig->readFile(true);

        if (nullptr != mMonitoredConfig) {

            applyConfig();
            return true;
        }
        mMonitoredConfig = std::unique_ptr<xrandrConfig>(new xrandrConfig(mConfig->clone()));
    }
    return false;
}

void XrandrManager::TabletSettingsChanged(const bool tablemode)
{
    if(tablemode)
    {
        setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::cloneScreenMode));
    } else {
        setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::extendScreenMode));
    }
    USD_LOG(LOG_DEBUG,"recv mode changed:%d", tablemode);
}

void XrandrManager::setScreenModeToClone()
{

    int rtResolution = 0;
    int bigestResolution = 0;
    bool hadFindFirstScreen = false;

    QString bigestPrimaryModeId;
    QString bigestModeId;
    QString secondScreen;

    KScreen::OutputPtr primaryOutput;// = mMonitoredConfig->data()->primaryOutput();

    if (false == checkPrimaryScreenIsSetable()) {
        return;
    }

    if (readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode::cloneScreenMode)) {
        return;
    }

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
        USD_LOG_SHOW_OUTPUT(output);

        if (false == output->isConnected()) {
            continue;
        }

        output->setEnabled(true);

        if (false == hadFindFirstScreen) {
            hadFindFirstScreen = true;
            primaryOutput = output;
            continue;
        }

        secondScreen = output->name().toLatin1().data();
        //遍历模式找出最大分辨率的克隆模式
        Q_FOREACH (auto primaryMode, primaryOutput->modes()) {
            Q_FOREACH (auto newOutputMode, output->modes()) {

                if (primaryMode->size().width() == newOutputMode->size().width() &&
                        primaryMode->size().height() == newOutputMode->size().height())
                {

                    rtResolution = primaryMode->size().width() * primaryMode->size().height();

                    if (rtResolution > bigestResolution){

                        bigestModeId = newOutputMode->id();
                        bigestPrimaryModeId = primaryMode->id();

                        bigestResolution = rtResolution;

                        output->setPos(QPoint(0,0));
                        primaryOutput->setPos(QPoint(0,0));

                        output->setCurrentModeId(bigestModeId);
                        primaryOutput->setCurrentModeId(bigestPrimaryModeId);

                        if (UsdBaseClass::isTablet()) {
                            output->setRotation(static_cast<KScreen::Output::Rotation>(getCurrentRotation()));
                        }

                        USD_LOG(LOG_DEBUG,"good..p_width:%d p_height:%d n_width:%d n_height:%d,rotation:%d",
                                primaryMode->size().width(),primaryMode->size().height(),
                                newOutputMode->size().width(), newOutputMode->size().height(),output->rotation());
                    }


                }
            }
        }
        USD_LOG_SHOW_OUTPUT(output);
    }

    if (0 == bigestResolution) {
       setScreenMode(metaEnum.key(UsdBaseClass::eScreenMode::extendScreenMode));
    } else {
       applyConfig();
    }
}

void XrandrManager::setScreenModeToFirst(bool isFirstMode)
{
    int posX = 0;
    int maxScreenSize = 0;
    bool hadFindFirstScreen = false;

    if (false == checkPrimaryScreenIsSetable()) {
        return;
    }

    if (isFirstMode){
        if (readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode::firstScreenMode)) {
            return;
        }
    } else {
        if (readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode::secondScreenMode)) {
            return;
        }
    }

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {

        USD_LOG_SHOW_OUTPUT(output);

        if (output->isConnected()) {
            output->setEnabled(true);
        } else {
            continue;
        }

        //找到第一个屏幕（默认为内屏）
        if (hadFindFirstScreen) {
                output->setEnabled(!isFirstMode);
        } else {
            hadFindFirstScreen = true;
            output->setEnabled(isFirstMode);
        }

        if (output->isEnabled()) {

            Q_FOREACH (auto Mode, output->modes()){

                if (Mode->size().width()*Mode->size().height() < maxScreenSize) {
                    continue;
                }

                maxScreenSize = Mode->size().width()*Mode->size().height();
                output->setCurrentModeId(Mode->id());
            }
            output->setPos(QPoint(posX,0));
            posX+=output->size().width();
        }

        USD_LOG_SHOW_OUTPUT(output);
    }

    applyConfig();
}

void XrandrManager::setScreenModeToExtend()
{
    int primaryX = 0;
    int screenSize = 0;
    int singleMaxWidth = 0;

    if (false == checkPrimaryScreenIsSetable()) {
        return;
    }

    if (readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode::extendScreenMode)) {
        return;
    }

    Q_FOREACH(const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
        screenSize = 0;
        singleMaxWidth = 0;
        USD_LOG_SHOW_OUTPUT(output);

        if (output->isConnected()){
            output->setEnabled(true);
        } else {
            output->setEnabled(false);
            continue;
        }

        Q_FOREACH (auto Mode, output->modes()){
            if (Mode->size().width()*Mode->size().height() > screenSize) {
                screenSize = Mode->size().width()*Mode->size().height();
                output->setCurrentModeId(Mode->id());
                if (Mode->size().width() > singleMaxWidth) {
                    singleMaxWidth = Mode->size().width();
                }
            }
         }

        if (UsdBaseClass::isTablet()) {
            output->setRotation(static_cast<KScreen::Output::Rotation>(getCurrentRotation()));
        }

         output->setPos(QPoint(primaryX,0));
         primaryX += singleMaxWidth;

         USD_LOG_SHOW_OUTPUT(output);
     }
    applyConfig();
}

void XrandrManager::setScreensParam(QString screensParam)
{

    USD_LOG(LOG_DEBUG,"param:%s", screensParam.toLatin1().data());

     std::unique_ptr<xrandrConfig> temp  = mMonitoredConfig->readScreensConfigFromDbus(screensParam);
     if (nullptr != temp) {
         mMonitoredConfig = std::move(temp);
     }

    applyConfig();
}

/* 
 * 设置显示模式
*/
void XrandrManager::setScreenMode(QString modeName)
{


    switch (metaEnum.keyToValue(modeName.toLatin1().data())) {
    case UsdBaseClass::eScreenMode::cloneScreenMode:
        USD_LOG(LOG_DEBUG,"ready set mode to %s",modeName.toLatin1().data());
        setScreenModeToClone();
        break;
    case UsdBaseClass::eScreenMode::firstScreenMode:
        USD_LOG(LOG_DEBUG,"ready set mode to %s",modeName.toLatin1().data());
        setScreenModeToFirst(true);
        break;
    case UsdBaseClass::eScreenMode::secondScreenMode:
        USD_LOG(LOG_DEBUG,"ready set mode to %s",modeName.toLatin1().data());
        setScreenModeToFirst(false);
        break;
    case UsdBaseClass::eScreenMode::extendScreenMode:
        USD_LOG(LOG_DEBUG,"ready set mode to %s",modeName.toLatin1().data());
        setScreenModeToExtend();
        break;

    default:
        Q_FOREACH (const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {
            USD_LOG_SHOW_OUTPUT(output);
        }
        USD_LOG(LOG_DEBUG,"set mode fail can't set to %s",modeName.toLatin1().data());
        return;
    }

    mDbus->mScreenMode = metaEnum.keyToValue(modeName.toLatin1().data());
    mMonitoredConfig->setScreenMode(modeName);
}

/*
 * 识别当前显示的模式
*/
UsdBaseClass::eScreenMode XrandrManager::discernScreenMode()
{
    bool firstScreenIsEnable = false;
    bool secondScreenIsEnable = false;
    bool hadFindFirstScreen = false;

    QPoint firstScreenQPoint;
    QPoint secondScreenQPoint;

    QSize firstScreenQsize;
    QSize secondScreenQsize;

    Q_FOREACH (const KScreen::OutputPtr &output, mMonitoredConfig->data()->outputs()) {

        if (output->isConnected()) {
            if (false == hadFindFirstScreen) {
                firstScreenIsEnable = output->isEnabled();
                firstScreenQPoint = output->pos();

                if (output->isEnabled()) {
                    firstScreenQsize = output->currentMode()->size();
                }
                hadFindFirstScreen = true;

            } else {
                secondScreenIsEnable = output->isEnabled();
                secondScreenQPoint = output->pos();
                if (secondScreenIsEnable) {
                    secondScreenQsize = output->currentMode()->size();
                }
                break;
            }
        }
    }

    if (true == firstScreenIsEnable && false == secondScreenIsEnable) {
        USD_LOG(LOG_DEBUG,"mode : firstScreenMode");
        return UsdBaseClass::eScreenMode::firstScreenMode;
    }

    if (false == firstScreenIsEnable && true == secondScreenIsEnable) {
        USD_LOG(LOG_DEBUG,"mode : secondScreenMode");
        return UsdBaseClass::eScreenMode::secondScreenMode;
    }

    if (firstScreenQPoint == secondScreenQPoint && firstScreenQsize==secondScreenQsize) {
        USD_LOG(LOG_DEBUG,"mode : cloneScreenMode");
        return UsdBaseClass::eScreenMode::cloneScreenMode;
    }

    USD_LOG(LOG_DEBUG,"mode : extendScreenMode");
    return UsdBaseClass::eScreenMode::extendScreenMode;
}

void XrandrManager::screenModeChangedSignal(int mode)
{
    USD_LOG(LOG_DEBUG,"mode:%d",mode);
}

void XrandrManager::screensParamChangedSignal(QString param)
{
    USD_LOG(LOG_DEBUG,"param:%s",param.toLatin1().data());
}

/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */
void XrandrManager::StartXrandrIdleCb()
{
    mAcitveTime->stop();

    mSaveConfigTimer = new QTimer(this);
    connect(mSaveConfigTimer, SIGNAL(timeout()), this, SLOT(SaveConfigTimerHandle()));

    //SetTouchscreenCursorRotation();

    USD_LOG(LOG_DEBUG,"StartXrandrIdleCb ok.");

    //    QMetaObject::invokeMethod(this, "getInitialConfig", Qt::QueuedConnection);
    connect(mKscreenInitTimer,  SIGNAL(timeout()), this, SLOT(getInitialConfig()));
    mKscreenInitTimer->start(1500);

    connect(mDbus, SIGNAL(setScreenModeSignal(QString)), this, SLOT(setScreenMode(QString)));
    connect(mDbus, SIGNAL(setScreensParamSignal(QString)), this, SLOT(setScreensParam(QString)));

#if 0
    QDBusInterface *modeChangedSignalHandle = new QDBusInterface(DBUS_XRANDR_NAME,DBUS_XRANDR_PATH,DBUS_XRANDR_INTERFACE,QDBusConnection::sessionBus(),this);

    if (modeChangedSignalHandle->isValid()) {
        connect(modeChangedSignalHandle, SIGNAL(screenModeChanged(int)), this, SLOT(screenModeChangedSignal(int)));

    } else {
        USD_LOG(LOG_ERR, "modeChangedSignalHandle");
    }

    QDBusInterface *screensChangedSignalHandle = new QDBusInterface(DBUS_XRANDR_NAME,DBUS_XRANDR_PATH,DBUS_XRANDR_INTERFACE,QDBusConnection::sessionBus(),this);

     if (screensChangedSignalHandle->isValid()) {
         connect(screensChangedSignalHandle, SIGNAL(screensParamChanged(QString)), this, SLOT(screensParamChangedSignal(QString)));
         //USD_LOG(LOG_DEBUG, "..");
     } else {
         USD_LOG(LOG_ERR, "screensChangedSignalHandle");
     }
#endif

}
