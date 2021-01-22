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
#include <QApplication>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QX11Info>

#include "xrandr-manager.h"

#define SETTINGS_XRANDR_SCHEMAS     "org.ukui.SettingsDaemon.plugins.xrandr"
#define XRANDR_ROTATION_KEY         "xrandr-rotations"
#define XRANDR_PRI_KEY              "primary"
#define XSETTINGS_SCHEMA            "org.ukui.SettingsDaemon.plugins.xsettings"
#define XSETTINGS_KEY_SCALING       "scaling-factor"

#define MAX_SIZE_MATCH_DIFF         0.05

typedef struct
{
    unsigned char *input_node;
    XIDeviceInfo dev_info;

}TsInfo;

XrandrManager::XrandrManager()
{
    time = new QTimer(this);
    mXrandrSetting = new QGSettings(SETTINGS_XRANDR_SCHEMAS);
    KScreen::Log::instance();
    QMetaObject::invokeMethod(this, "getInitialConfig", Qt::QueuedConnection);
}

void XrandrManager::getInitialConfig()
{
    connect(new KScreen::GetConfigOperation, &KScreen::GetConfigOperation::finished,
            this, [this](KScreen::ConfigOperation* op) {
        if (op->hasError()) {
            qDebug() << "Error getting initial configuration" << op->errorString();
            return;
        }
        if (mMonitoredConfig) {
            if(mMonitoredConfig->data()){
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
        //qDebug()<<mMonitoredConfig->data().data() << "is ready";
        monitorsInit();
        applyConfig();
    });
}

XrandrManager::~XrandrManager()
{

    if(time)
        delete time;

    if(mXrandrSetting)
        delete mXrandrSetting;
}

bool XrandrManager::XrandrManagerStart()
{
    qDebug("Xrandr Manager Start");
    connect(time, &QTimer::timeout, this,&XrandrManager::StartXrandrIdleCb);
    time->start();
    return true;
}

void XrandrManager::XrandrManagerStop()
{
    qDebug("Xrandr Manager Stop");
}

/*查找触摸屏设备ID*/
static bool
find_touchscreen_device(Display* display, XIDeviceInfo *dev)
{
    int i, j;
    if (dev->use != XISlavePointer)
        return false;
    if (!dev->enabled)
    {
        qDebug("This device is disabled.");
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

    printf("w_diff is %f, h_diff is %f\n", w_diff, h_diff);

    if (w_diff < MAX_SIZE_MATCH_DIFF && h_diff < MAX_SIZE_MATCH_DIFF)
        return true;
    else
        return false;
}

/* Here to run command xinput
   更新触摸屏触点位置
*/
void doAction (char *input_name, char *output_name)
{
    char buff[100];
    sprintf(buff, "xinput --map-to-output \"%s\" \"%s\"", input_name, output_name);

    printf("buff is %s\n", buff);

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

    if ( major >= 1 && minor >= 5)
    {
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
void XrandrManager::RotationChangedEvent(QString key)
{
    int angle;

    if(key != XRANDR_ROTATION_KEY)
        return;

    angle = mXrandrSetting->getEnum(XRANDR_ROTATION_KEY);
    qDebug()<<"angle = "<<angle;

    const KScreen::OutputList outputs = mMonitoredConfig->data()->outputs();
    for(auto output : outputs){
        if (!output->isConnected() || !output->isEnabled() || !output->currentMode()) {
            continue;
        }
        output->setRotation(static_cast<KScreen::Output::Rotation>(angle));
        qDebug()<<output->rotation() <<output->name();
    }
    doApplyConfig(mMonitoredConfig->data());
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

    refreshConfig();
}

void XrandrManager::refreshConfig()
{
    //setMonitorForChanges(false);
    mConfigDirty = false;
    KScreen::ConfigMonitor::instance()->addConfig(mMonitoredConfig->data());

    connect(new KScreen::SetConfigOperation(mMonitoredConfig->data()),
                &KScreen::SetConfigOperation::finished,
            this, [this]() {
        //qDebug() << "Config applied";
        if (mConfigDirty) {
            // Config changed in the meantime again, apply.
            doApplyConfig(mMonitoredConfig->data());
        }
    });
}

void XrandrManager::applyKnownConfig()
{
    std::unique_ptr<xrandrConfig> readInConfig = mMonitoredConfig->readFile();

    if (readInConfig) {
        doApplyConfig(std::move(readInConfig));
    } else {
        //qDebug() << "Loading failed, falling back to the ideal config" << mMonitoredConfig->id();
        applyIdealConfig();
    }
}

void XrandrManager::applyConfig()
{
    if (!mMonitoredConfig){
        qDebug()<<"mMonitoredConfig  is nullptr";
        return;
    }
    if (mMonitoredConfig->fileExists()) {
        applyKnownConfig();
        return;
    }
    applyIdealConfig();
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
    QMetaObject::invokeMethod(this, "getInitialConfig", Qt::QueuedConnection);
}

void XrandrManager::outputRemoved(int outputId)
{
     applyConfig();
}

void XrandrManager::primaryOutputChanged(const KScreen::OutputPtr &output) {
    Q_ASSERT(mMonitoredConfig->data());
    int index = output.isNull() ? 0 : output->id();
    if(index != 0){
//        QString str = xrandrOutput::metadata(mConfig->primaryOutput())[QStringLiteral("fullname")].toString();
//        qDebug()<<__func__<<str;
    }
}


void XrandrManager::monitorsInit()
{
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
            this, &XrandrManager::outputAdded);
    connect(mConfig.data(), &KScreen::Config::outputRemoved,
            this, &XrandrManager::outputRemoved);
    connect(mConfig.data(), &KScreen::Config::primaryOutputChanged,
            this, &XrandrManager::primaryOutputChanged);
}

/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */
void XrandrManager::StartXrandrIdleCb()
{

    time->stop();
    SetTouchscreenCursorRotation();
    mScreen = nullptr;
    mScreen = QApplication::primaryScreen();
    if(!mScreen)
        mScreen = QApplication::screens().at(0);

    connect(mXrandrSetting,SIGNAL(changed(QString)),this,SLOT(RotationChangedEvent(QString)));
    connect(mScreen, &QScreen::orientationChanged, this,
            &XrandrManager::orientationChangedProcess);
}
