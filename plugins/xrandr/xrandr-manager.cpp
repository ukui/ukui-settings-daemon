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
#include <QCoreApplication>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include "xrandr-manager.h"


#define SETTINGS_XRANDR_SCHEMAS "org.ukui.SettingsDaemon.plugins.xrandr"
#define XRANDR_ROTATION_KEY     "xrandr-rotations"

XrandrManager *XrandrManager::mXrandrManager = nullptr;

XrandrManager::XrandrManager()
{
    time = new QTimer(this);
    mXrandrSetting = new QGSettings(SETTINGS_XRANDR_SCHEMAS);
}

XrandrManager::~XrandrManager()
{
    if(mXrandrManager){
        delete mXrandrManager;
        mXrandrManager = nullptr;
    }
    if(time)
        delete time;

    if(mXrandrSetting)
        delete mXrandrSetting;
}

XrandrManager* XrandrManager::XrandrManagerNew()
{
    if (nullptr == mXrandrManager)
        mXrandrManager = new XrandrManager();
    return mXrandrManager;
}

bool XrandrManager::XrandrManagerStart()
{
    qDebug("Xrandr Manager Start");
    connect(time,SIGNAL(timeout()),this,SLOT(StartXrandrIdleCb()));

    time->start();

    return true;
}

void XrandrManager::XrandrManagerStop()
{
    qDebug("Xrandr Manager Stop");
}


/**
 * @name ReadMonitorsXml();
 * @brief 读取monitors.xml文件
 */
bool XrandrManager::ReadMonitorsXml()
{
    int mNum = 0;
    QString homePath = getenv("HOME");
    QString monitorFile = homePath+"/.config/monitors.xml";
    QFile file(monitorFile);
    if(!file.open(QIODevice::ReadOnly))
            return false;

    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return false;
    }
    file.close();

    XmlFileTag.clear();
    mIntDate.clear();

    QDomElement root=doc.documentElement(); //返回根节点
    //qDebug()<<root.nodeName();
    QDomNode n=root.firstChild();

    while(!n.isNull())
    {
        if(n.isElement()){
            QDomElement e=n.toElement();
            QDomNodeList list=e.childNodes();

            for(int i=0;i<list.count();i++){
                QDomNode node=list.at(i);

                if(node.isElement()){
                    QDomNodeList e2 = node.childNodes();

                    if(node.toElement().tagName() == "clone"){
                        XmlFileTag.insert("clone",node.toElement().text());
                        qDebug()<<"clone:"<<node.toElement().tagName()<<node.toElement().text();
                    }else if("output" == node.toElement().tagName()){
                        XmlFileTag.insert("name",node.toElement().attribute("name"));
                        for(int j=0;j<e2.count();j++){
                            QDomNode node2 = e2.at(j);

                            if(node2.toElement().tagName() == "width")
                                XmlFileTag.insert("width",node2.toElement().text());
                            else if(node2.toElement().tagName() == "height")
                                XmlFileTag.insert("height",node2.toElement().text());
                            else if("x" == node2.toElement().tagName())
                                XmlFileTag.insert("x",node2.toElement().text());
                            else if("y" == node2.toElement().tagName())
                                XmlFileTag.insert("y",node2.toElement().text());
                            else if(node2.toElement().tagName() == "primary")
                                XmlFileTag.insert("primary",node2.toElement().text());

                        }
                        mNum++;
                    }
                }
            }
        }
        n = n.nextSibling();
    }
    qDebug()<<"mNum = "<<mNum;
    mIntDate.insert("XmlNum",mNum);
    return true;
}

/**
 * @name SetScreenSize();
 * @brief 设置单个屏幕分辨率
 */
bool XrandrManager::SetScreenSize(Display  *dpy, Window root, int width, int height)
{
    SizeID          current_size;
    Rotation        current_rotation;
    XRRScreenSize   *sizes;
    XRRScreenConfiguration *sc;
    int     nsize = 0;
    int     size = -1;
    int     rot  = -1;
    short   current_rate;
    double  rate = -1;
    int     reflection = 0;

    sc = XRRGetScreenInfo (dpy, root);
    if (sc == NULL){
        qDebug("Screen configuration is Null");
        return false;
    }
    /* 配置当前配置 */
    current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);
    sizes = XRRConfigSizes(sc, &nsize);

    for (size = 0;size < nsize; size++)
    {
        if (sizes[size].width == width && sizes[size].height == height)
            break;
    }

    if (size >= nsize) {
        qWarning("Size %dx%d not found in available modes\n", width, height);
        return false;
    } else if (size < 0)
        size = current_size;

    if (rot < 0) {
        for (rot = 0; rot < 4; rot++)
            if (1 << rot == (current_rotation & 0xf))
                break;
    }
    current_rate = XRRConfigCurrentRate (sc);
    if (rate < 0) {
        if (size == current_size)
            rate = current_rate;
        else
            rate = 0;
    }
    XSelectInput (dpy, root, StructureNotifyMask);
    Rotation rotation = 1 << rot;
    XRRSetScreenConfigAndRate (dpy, sc, root, (SizeID) size,
                               (Rotation) (rotation | reflection),
                               rate, CurrentTime);
    XRRFreeScreenConfigInfo(sc);
    return true;
}

/**
 * @name AutoConfigureOutputs();
 * @brief 自动配置输出,在进行硬件HDMI屏幕插拔时
 */
void XrandrManager::AutoConfigureOutputs (XrandrManager *manager,
                                          unsigned int timestamp)
{
    int i, x;
    GList *l;
    GList *just_turned_on;
    MateRRConfig *config;
    MateRROutputInfo **outputs;
    gboolean applicable;

    config = mate_rr_config_new_current (manager->mScreen, NULL);
    just_turned_on = NULL;
    outputs = mate_rr_config_get_outputs (config);

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (mate_rr_output_info_is_connected (output) && !mate_rr_output_info_is_active (output)) {
                mate_rr_output_info_set_active (output, TRUE);
                mate_rr_output_info_set_rotation (output, MATE_RR_ROTATION_0);
                just_turned_on = g_list_prepend (just_turned_on, GINT_TO_POINTER (i));
        } else if (!mate_rr_output_info_is_connected (output) && mate_rr_output_info_is_active (output))
                mate_rr_output_info_set_active (output, FALSE);
    }

    /* Now, lay out the outputs from left to right.  Put first the outputs
     * which remained on; put last the outputs that were newly turned on.
     * 现在，从左到右布置输出。 首先整理目前存在的输出, 最后放置新打开的输出。
     */
    x = 0;

    /* First, outputs that remained on
     * 首先，输出保持不变
     */
    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (g_list_find (just_turned_on, GINT_TO_POINTER (i)))
                continue;

        if (mate_rr_output_info_is_active (output)) {
            int width=0, height=0;
            char *name;

            g_assert (mate_rr_output_info_is_connected (output));
            name = mate_rr_output_info_get_name(output);

            mate_rr_output_info_get_geometry (output, NULL, NULL, &width, &height);
            mate_rr_output_info_set_geometry (output, x, 0, width, height);

            x += width;
        }
    }

    /* Second, outputs that were newly-turned on
     * 第二，新打开的输出
     */
    for (l = just_turned_on; l; l = l->next) {
        MateRROutputInfo *output;
        int width=0, height=0;

        i = GPOINTER_TO_INT (l->data);
        output = outputs[i];
        g_assert (mate_rr_output_info_is_active (output) && mate_rr_output_info_is_connected (output));

        /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
        width = mate_rr_output_info_get_preferred_width (output);
        height = mate_rr_output_info_get_preferred_height (output);

        mate_rr_output_info_set_geometry (output, x, 0, width, height);
        x += width;
    }

    /* Check if we have a large enough framebuffer size.  If not, turn off
     * outputs from right to left until we reach a usable size.
     * 检查我们是否有足够大的帧缓冲区大小。如果没有，从右到左关闭输出，直到达到可用大小为止。
     */
    just_turned_on = g_list_reverse (just_turned_on); /* now the outputs here are from right to left */
    l = just_turned_on;
    while (1) {
        MateRROutputInfo *output;
        applicable = mate_rr_config_applicable (config, manager->mScreen, NULL);
        if (applicable)
                break;
        if (l) {
            i = GPOINTER_TO_INT (l->data);
            l = l->next;

            output = outputs[i];
            mate_rr_output_info_set_active (output, FALSE);
        } else
            break;
    }
    /* Apply the configuration!
     * 应用配置
     */
    if (applicable)
        mate_rr_config_apply_with_time (config, manager->mScreen, timestamp, NULL);

    g_list_free (just_turned_on);
    g_object_unref (config);
}

/*查找触摸屏设备ID*/
static gboolean find_touchscreen_device(Display* display, XIDeviceInfo *dev)
{
    int i, j;
    if (dev->use != XISlavePointer)
        return FALSE;
    if (!dev->enabled)
    {
        qDebug("This device is disabled.");
        return FALSE;
    }

    for (i = 0; i < dev->num_classes; i++)
    {
        if (dev->classes[i]->type == XITouchClass)
        {
            XITouchClassInfo *t = (XITouchClassInfo*)dev->classes[i];

            if (t->mode == XIDirectTouch)
            return TRUE;
        }
    }
    return FALSE;
}

/*检测到旋转，更改触摸屏鼠标光标位置*/
void SetTouchscreenCursor(float (&matrix)[9])
{
    Display *xdisplay = XOpenDisplay(NULL);
    Atom property_atom;
    XIDeviceInfo *info;
    int i, ndevices;
    int ts_device_id = 0;

    info = XIQueryDevice(xdisplay, XIAllDevices, &ndevices);

    for (i = 0; i < ndevices; i++)
    {
        if (find_touchscreen_device(xdisplay, &info[i])) {
            ts_device_id = info[i].deviceid;
        } 
    }
    if (ts_device_id == 0) {
        qDebug("device id is 0.");
        return;
    }
    
    property_atom = XInternAtom (xdisplay, "Coordinate Transformation Matrix", True);
    if (!property_atom)
        return;
    Atom type = XInternAtom (xdisplay, "FLOAT", False);
    XIChangeProperty (xdisplay, ts_device_id, property_atom, type,
                        32, XIPropModeReplace, (unsigned char *)(&matrix[0]), 9);
    XCloseDisplay(xdisplay);
}

bool XrandrManager::ApplyConfigurationFromFilename (XrandrManager *manager,
                                                    const char    *filename,
                                                    unsigned int   timestamp)
{
    bool success;
    success = mate_rr_config_apply_from_filename_with_time (manager->mScreen, filename, timestamp, NULL);
    if (success)
        return true;
    return  false;
}

void XrandrManager::RestoreBackupConfiguration (XrandrManager  *manager,
                                                const char     *backup_filename,
                                                const char     *intended_filename,
                                                unsigned int    timestamp)
{
    if (rename (backup_filename, intended_filename) == 0){
        ApplyConfigurationFromFilename (manager, intended_filename, timestamp);
    }
    unlink (backup_filename);
}

bool XrandrManager::ApplyStoredConfigurationAtStartup(XrandrManager *manager,
                                                      unsigned int   timestamp)
{
    bool success;
    char *backup_filename;
    char *intended_filename;

    backup_filename = mate_rr_config_get_backup_filename ();
    intended_filename = mate_rr_config_get_intended_filename ();

    success = ApplyConfigurationFromFilename(manager, backup_filename, timestamp);
    if(success)
        manager->RestoreBackupConfiguration (manager, backup_filename, intended_filename, timestamp);

    success = ApplyConfigurationFromFilename (manager, intended_filename, timestamp);
    free (backup_filename);
    free (intended_filename);
    return success;
}

void SetTouchscreenCursorRotation(MateRRScreen *screen, QGSettings *settngs)
{
    /* 添加触摸屏鼠标设置 */
    MateRRConfig *result;
    MateRROutputInfo **outputs;
    int i;
    result = mate_rr_config_new_current (screen, NULL);
    outputs = mate_rr_config_get_outputs (result);
    for(i=0; outputs[i] != NULL;++i)
    {
        MateRROutputInfo *info = outputs[i];
        if(mate_rr_output_info_is_connected (info)){
            int rotation = mate_rr_output_info_get_rotation(info);
            qDebug("rotation is %d.", rotation);
            switch(rotation){
                case MATE_RR_ROTATION_0:
                {
                    float full_matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                    SetTouchscreenCursor(full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_90:
                {
                    float full_matrix[9] = {0, -1, 1, 1, 0, 0, 0, 0, 1};
                    SetTouchscreenCursor(full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_180:
                {
                    float full_matrix[9] = {-1,  0, 1, 0, -1, 1, 0, 0, 1};
                    SetTouchscreenCursor(full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_270:
                {
                    float full_matrix[9] = {0, 1, 0, -1, 0, 1, 0, 0, 1};
                    SetTouchscreenCursor(full_matrix);
                    break;
                }
                default:
                {
                    float full_matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                    SetTouchscreenCursor(full_matrix);
                }
            }
        }
    }
    g_object_unref (result);
}

/**
 * @brief XrandrManager::OnRandrEvent : 屏幕事件回调函数
 * @param screen
 * @param data
 */
void XrandrManager::OnRandrEvent(MateRRScreen *screen, gpointer data)
{
    unsigned int change_timestamp, config_timestamp;
    XrandrManager *manager = (XrandrManager*) data;

    /* 获取更改时间 和 配置时间 */
    mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

    if (change_timestamp >= config_timestamp) {
        /* The event is due to an explicit configuration change.
         * If the change was performed by us, then we need to do nothing.
         * If the change was done by some other X client, we don't need
         * to do anything, either; the screen is already configured.
         */
        qDebug()<<"Ignoring event since change >= config";
    } else {
        char *intended_filename;
        bool  success;
        intended_filename = mate_rr_config_get_intended_filename ();
        success = ApplyConfigurationFromFilename (manager, intended_filename, config_timestamp);
        free (intended_filename);
        if(!success)
            manager->AutoConfigureOutputs (manager, config_timestamp);
    }
    /* 添加触摸屏鼠标设置 */
    SetTouchscreenCursorRotation(screen, manager->mXrandrSetting);
}

/*监听旋转键值回调 并设置旋转角度*/
void XrandrManager::RotationChangedEvent(QString key)
{
    int angle, i;
    MateRRConfig        *result;
    MateRROutputInfo    **outputs;
    MateRRRotation      rotation;
    unsigned int config_timestamp;
    if(key != XRANDR_ROTATION_KEY)
        return;

    angle = mXrandrSetting->getEnum(XRANDR_ROTATION_KEY);
    qDebug()<<"angle = "<<angle;
    /*switch (angle) {
        case 0:
            rotation = MATE_RR_ROTATION_0;
        break;
        case 1:
            rotation = MATE_RR_ROTATION_90;
        break;
        case 2:
            rotation = MATE_RR_ROTATION_180;
        break;
        case 3:
            rotation = MATE_RR_ROTATION_270;
        break;
    }*/
    //mate_rr_screen_get_timestamps (mScreen, nullptr, &config_timestamp);
    result = mate_rr_config_new_current (mScreen, NULL);
    outputs = mate_rr_config_get_outputs (result);
    for (i = 0; outputs[i] != NULL; ++i) {
        MateRROutputInfo *info = outputs[i];
        if (mate_rr_output_info_is_connected (info)) {
            QString name = mate_rr_output_info_get_name(info);
            qDebug()<<"name = " << name;
            switch(angle) {
                case 0:
                    QProcess::execute("xrandr --output "+ name + " --rotate normal");
                break;
                case 1:
                    QProcess::execute("xrandr --output "+ name + " --rotate left");
                break;
                case 2:
                    QProcess::execute("xrandr --output "+ name + " --rotate inverted");
                break;
                case 3:
                    QProcess::execute("xrandr --output "+ name + " --rotate right");
                break;
            }
            //mate_rr_output_info_set_rotation (info, rotation);
        }
    }
    //mate_rr_config_apply_with_time (result, mScreen, config_timestamp, NULL);
    g_object_unref (result);
}

/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */
void XrandrManager::StartXrandrIdleCb()
{
    Display     *dpy;
    Window      root;
    QString ScreenName;
    int ScreenNum = 1;
    int width,  height;

    time->stop();
    mScreen = mate_rr_screen_new (gdk_screen_get_default (),NULL);
    if(mScreen == nullptr){
        qDebug()<<"Could not initialize the RANDR plugin";
        return;
    }
    g_signal_connect (mScreen, "changed", G_CALLBACK (OnRandrEvent), this);

    connect(mXrandrSetting,SIGNAL(changed(QString)),this,SLOT(RotationChangedEvent(QString)));

    /*设置虚拟机分辨率*/
    ScreenNum  = QApplication::screens().length();
    ScreenName = QApplication::primaryScreen()->name();
    if((ScreenNum==1) && (ScreenName == "Virtual1")){
        int screen, XmlNum;
        dpy = XOpenDisplay(0);
        screen = DefaultScreen(dpy);
        root = RootWindow(dpy, screen);
        ReadMonitorsXml();
        XmlNum = mIntDate.value("XmlNum");
        for(int i=0;i<XmlNum;i++){
            QString DisplayName = XmlFileTag.values("name")[i];
            if(ScreenName == DisplayName){
                width = XmlFileTag.values("width")[i].toLatin1().toInt();
                height = XmlFileTag.values("height")[i].toLatin1().toInt();
            }
        }
        SetScreenSize(dpy, root, width, height);
        XCloseDisplay (dpy);
    }
    /*登录读取注销配置*/
    ApplyStoredConfigurationAtStartup(this,GDK_CURRENT_TIME);

    /* 添加触摸屏鼠标设置 */
    SetTouchscreenCursorRotation(mScreen, mXrandrSetting);
}
