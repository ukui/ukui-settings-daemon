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
#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QObject>
#include <QGSettings/qgsettings.h>
#include <QTimer>
#include <glib.h>
#include <geoclue.h>

#include "color-state.h"
#include "color-profiles.h"

#define USD_COLOR_TEMPERATURE_MIN               1000    /* Kelvin */
#define USD_COLOR_TEMPERATURE_DEFAULT           6500    /* Kelvin, is RGB [1.0,1.0,1.0] */
#define USD_COLOR_TEMPERATURE_MAX               10000   /* Kelvin */

class ColorManager : public QObject
{
    Q_OBJECT
private:
    ColorManager();
    ColorManager(ColorManager&)=delete;
    ColorManager&operator=(const ColorManager&)=delete;

public:
    ~ColorManager();
    static ColorManager* ColorManagerNew();
    bool ColorManagerStart();
    void ColorManagerStop();

    void StartGeoclue();
    void StopGeoclue();
    static void OnGeoclueSimpleReady (GObject *source_object,
                                      GAsyncResult *res,
                                      gpointer   user_data);
    static void OnLocationNotify (GClueSimple *simple,
                                  GParamSpec  *pspec,
                                  gpointer     user_data);

    static void NightLightRecheck(ColorManager *manager);
    GDateTime *NightLightGetDateTimeNow();

    void PollSmoothCreate (double temperature);
    void PollSmoothDestroy ();
    void NightLightSetTemperatureInternal (double temperature);
    void NightLightSetTemperature(double temperature);
    static bool NightLightSmoothCb (ColorManager *manager);
    void NightLightSetActive(bool active);
    bool UpdateCachedSunriseSunset();
    void ReadKwinColorTempConfig();

public Q_SLOTS:
    void SettingsChangedCb(QString);
    void checkTime();
private:
    static ColorManager *mColorManager;
    ColorProfiles       *mColorProfiles;
    ColorState          *mColorState;

    QGSettings *settings;
    QGSettings *gtk_settings;
    QGSettings *qt_settings;
    bool        forced;
    GSource    *source;
    bool        geoclue_enabled;
    bool        smooth_enabled;
    bool        cached_active;
    double      cached_sunrise;
    double      cached_sunset;
    double      cached_temperature;
    bool        disabled_until_tmw;
    GDateTime  *disabled_until_tmw_dt;
    GDateTime  *datetime_override;
    GTimer     *smooth_timer;
    QTimer      *m_NightChecktimer;
    guint       smooth_id;
    double      smooth_target_temperature;
    GCancellable  *cancellable;
    GClueClient   *geoclue_client;
    GClueSimple   *geoclue_simple;
    QHash<QString, QVariant> mNightConfig;
};

#endif // COLORMANAGER_H
