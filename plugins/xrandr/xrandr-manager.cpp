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
#include "xrandr-manager.h"
#include <syslog.h>

XrandrManager *XrandrManager::mXrandrManager = nullptr;

XrandrManager::XrandrManager()
{
    time = new QTimer(this);
}

XrandrManager::~XrandrManager()
{
    if(mXrandrManager){
        delete mXrandrManager;
        mXrandrManager = nullptr;
    }
    if(time){
        delete time;
        time = nullptr;
    }
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
    syslog(LOG_ERR,"Xrandr Manager Stop");
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

/*检测到旋转，更改触摸屏鼠标光标位置*/
void SetTouchscreenCursor(void *date)
{
        Display *xdisplay = XOpenDisplay(NULL);
        Atom property_atom;
        property_atom = XInternAtom (xdisplay, "Coordinate Transformation Matrix", True);
        if (!property_atom)
            return;
        Atom type = XInternAtom (xdisplay, "FLOAT", False);
        XIChangeProperty (xdisplay, XITouchClass, property_atom, type,
                          32, XIPropModeReplace, (unsigned char *)date, 9);
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

void SetTouchscreenCursorRotation(MateRRScreen *screen)
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
            switch(rotation){
                case MATE_RR_ROTATION_0:
                {
                    float full_matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                    SetTouchscreenCursor(&full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_90:
                {
                    float full_matrix[9] = {0, -1, 1, 1, 0, 0, 0, 0, 1};
                    SetTouchscreenCursor(&full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_180:
                {
                    float full_matrix[9] = {-1,  0, 1, 0, -1, 1, 0, 0, 1};
                    SetTouchscreenCursor(&full_matrix);
                    break;
                }
                case MATE_RR_ROTATION_270:
                {
                    float full_matrix[9] = {0, 1, 0, -1, 0, 1, 0, 0, 1};
                    SetTouchscreenCursor(&full_matrix);
                    break;
                }
                default:
                {
                    float full_matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                    SetTouchscreenCursor(&full_matrix);
                }
            }
        }
    }
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
    SetTouchscreenCursorRotation(screen);
}

/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */
void XrandrManager::StartXrandrIdleCb()
{
    time->stop();

    mScreen = mate_rr_screen_new (gdk_screen_get_default (),NULL);
    if(mScreen == nullptr){
        qDebug()<<"Could not initialize the RANDR plugin";
        return;
    }
    g_signal_connect (mScreen, "changed", G_CALLBACK (OnRandrEvent), this);

    /*登录读取注销配置*/
    ApplyStoredConfigurationAtStartup(this,GDK_CURRENT_TIME);

    /* 添加触摸屏鼠标设置 */
    SetTouchscreenCursorRotation(mScreen);
}
