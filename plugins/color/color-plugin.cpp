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
#include <QDebug>
#include "color-plugin.h"

PluginInterface *ColorPlugin::mInstance      = nullptr;
ColorManager    *ColorPlugin::mColorManager  = nullptr;

ColorPlugin::ColorPlugin()
{
    qDebug()<<"Color Plugin initializing";
    if(nullptr == mColorManager)
        mColorManager = ColorManager::ColorManagerNew();
}

ColorPlugin::~ColorPlugin()
{
    if(mColorManager)
        delete mColorManager;
    if(mInstance)
        delete mInstance;
}
void ColorPlugin::activate()
{
    qDebug("activating Color plugins");
    bool res = mColorManager->ColorManagerStart();
    if(!res)
        qWarning("Unable to start Color manager!");
}

void ColorPlugin::deactivate()
{
    qDebug("Deactivating Color plugin");
    mColorManager->ColorManagerStop();
}

PluginInterface *ColorPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new ColorPlugin();

    return mInstance;
}

PluginInterface *createSettingsPlugin()
{
    return ColorPlugin::getInstance();
}

