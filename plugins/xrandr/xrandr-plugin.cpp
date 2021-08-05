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

    USD_LOG(LOG_DEBUG, "activating Xrandr plugins");

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
