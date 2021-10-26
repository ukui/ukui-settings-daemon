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
#include "usd_base_class.h"

PluginInterface *ColorPlugin::mInstance      = nullptr;
ColorManager    *ColorPlugin::mColorManager  = nullptr;

ColorPlugin::ColorPlugin()
{
    if(UsdBaseClass::isLoongarch()){
        return;
    }
    USD_LOG (LOG_DEBUG  , "new %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);

    if(nullptr == mColorManager)
        mColorManager = ColorManager::ColorManagerNew();
}

ColorPlugin::~ColorPlugin()
{
    if(mColorManager)
        delete mColorManager;
}
void ColorPlugin::activate()
{
    if(UsdBaseClass::isLoongarch()){
        return;
    }
    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);

    bool res = mColorManager->ColorManagerStart();
    if(!res)
        qWarning("Unable to start Color manager!");
}

void ColorPlugin::deactivate()
{
    USD_LOG (LOG_DEBUG, "deactivate %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);
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

