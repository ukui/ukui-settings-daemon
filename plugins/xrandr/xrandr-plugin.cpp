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
#include "xrandr-plugin.h"
#include "clib-syslog.h"

PluginInterface *XrandrPlugin::mInstance      = nullptr;
XrandrManager   *XrandrPlugin::mXrandrManager = nullptr;
/*
 *《》《》《》《》屏幕处理流程：
 * 控制面板（UCC）
 * 用户配置服务（USD）
 * 屏幕模式切换（KDS）
 *《》《》《》《》将用户屏幕分为五个文件夹:
 *（share/kscreen/）:记录用户设置最后一次的设置。
 *（share/kscreen/first）:记录用户在内屏模式下的设置
 *（share/kscreen/clone）:记录用户在克隆模式下的设置
 *（share/kscreen/extend）:记录用户在拓展模式下的设置
 *（share/kscreen/other）:记录用户在其他屏模式下的设置
 * 《》《》《》《》四个模式缺省设置：
 * 内屏模式：以最大分辨率进行显示内屏并将内屏坐标迁移到0，0，disable外屏。
 * 克隆模式：以最大分辨率进行显示克隆模式，enable全部已链接屏幕。
 * 拓展模式：以最大分辨率显示拓展模式，enable全部已链接屏幕，并将外屏迁移到内屏的右侧。
 * 其他屏幕：以最大分辨率显示外屏，并且将外屏迁移到0，0坐标。
 * 《》《》《》《》交互逻辑：
 * KDS：调用xrandr组件的模式切换dbus，传入模式字段（first，exten，clone，extend）进行处理。
 * UCC：调用底层库，发送信号
*/
XrandrPlugin::XrandrPlugin()
{
    if (UsdBaseClass::isWayland()) {
        QString str = "/usr/bin/peony-qt-desktop -b";
        QProcess::startDetached(str);

        USD_LOG(LOG_DEBUG, "disable xrandr in wayland...");
        return;
    }

    USD_LOG(LOG_DEBUG, "Xrandr Plugin initializing!:%s",QGuiApplication::platformName().toLatin1().data());
    if(nullptr == mXrandrManager)
        mXrandrManager = new XrandrManager();
}

XrandrPlugin::~XrandrPlugin()
{
    if(mXrandrManager)
        delete mXrandrManager;
    if(mInstance)
        delete mInstance;
}

void XrandrPlugin::activate()
{
    bool res = QGuiApplication::platformName().startsWith(QLatin1String("wayland"));

    if (true == res) {
        USD_LOG(LOG_DEBUG, "wayland need't usd to manage the screen");
        return;
    }

    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);


    res = mXrandrManager->XrandrManagerStart();
    if(!res) {
        USD_LOG(LOG_ERR,"Unable to start Xrandr manager!");
    }

}

PluginInterface *XrandrPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new XrandrPlugin();

    return mInstance;
}

void XrandrPlugin::deactivate()
{
    USD_LOG(LOG_ERR,"Deactivating Xrandr plugin");
    mXrandrManager->XrandrManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return XrandrPlugin::getInstance();
}
