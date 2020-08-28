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
#ifndef A11YSETTINGSPLUGIN_H
#define A11YSETTINGSPLUGIN_H
#include "plugin-interface.h"
#include "a11ysettingsmanager.h"

class A11ySettingsPlugin : public PluginInterface
{
public:
    ~A11ySettingsPlugin();
    static PluginInterface* getInstance();

    void activate();
    void deactivate();

private:
    A11ySettingsPlugin();
    A11ySettingsPlugin(A11ySettingsPlugin&) = delete;

    A11ySettingsManager*        settingsManager;
    static PluginInterface*     mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // A11YSETTINGSPLUGIN_H
