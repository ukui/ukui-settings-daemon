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
#include "color-manager.h"
#include <math.h>
#include <QTimer>

#define PLUGIN_COLOR_SCHEMA         "org.ukui.SettingsDaemon.plugins.color"
#define COLOR_KEY_LAST_COORDINATES  "night-light-last-coordinates"
#define COLOR_KEY_ENABLED           "night-light-enabled"
#define COLOR_KEY_ALLDAY            "night-light-allday"
#define COLOR_KEY_AUTO_THEME        "theme-schedule-automatic"
#define COLOR_KEY_TEMPERATURE       "night-light-temperature"
#define COLOR_KEY_AUTOMATIC         "night-light-schedule-automatic"
#define COLOR_KEY_AUTOMATIC_FROM    "night-light-schedule-automatic-from"
#define COLOR_KEY_AUTOMATIC_TO      "night-light-schedule-automatic-to"
#define COLOR_KEY_FROM              "night-light-schedule-from"
#define COLOR_KEY_TO                "night-light-schedule-to"

#define GTK_THEME_SCHEMA            "org.mate.interface"
#define GTK_THEME_KEY               "gtk-theme"

#define QT_THEME_SCHEMA             "org.ukui.style"
#define QT_THEME_KEY                "style-name"

#define USD_NIGHT_LIGHT_SCHEDULE_TIMEOUT    5       /* seconds */
#define USD_NIGHT_LIGHT_POLL_TIMEOUT        60      /* seconds */
#define USD_NIGHT_LIGHT_POLL_SMEAR          1       /* hours */
#define USD_NIGHT_LIGHT_SMOOTH_SMEAR        5.f     /* seconds */

#define USD_FRAC_DAY_MAX_DELTA              (1.f/60.f)     /* 1 minute */
#define USD_TEMPERATURE_MAX_DELTA           (10.f)

#define DESKTOP_ID "ukui-color-panel"
#include <sys/timerfd.h>
#include <unistd.h>

enum theme_status_switch{
    white_mode,
    black_mode
};


ColorManager *ColorManager::mColorManager = nullptr;

ColorManager::ColorManager()
{
    forced = false;
    smooth_id = 0;
    smooth_timer = nullptr;
    disabled_until_tmw = false;
    datetime_override = NULL;
    geoclue_enabled = true;
    smooth_enabled  = true;
    cached_sunrise  = -1.f;
    cached_sunset   = -1.f;
    cached_temperature = USD_COLOR_TEMPERATURE_DEFAULT;
    settings = new QGSettings (PLUGIN_COLOR_SCHEMA);
    gtk_settings = new QGSettings (GTK_THEME_SCHEMA);
    qt_settings = new QGSettings (QT_THEME_SCHEMA);
    mColorState    = new ColorState();
    mColorProfiles = new ColorProfiles();
}

ColorManager::~ColorManager()
{
    if(mColorManager)
        delete mColorManager;
    if(settings)
        delete settings;
    if(gtk_settings)
        delete gtk_settings;
    if(qt_settings)
        delete qt_settings;
    if(mColorState)
        delete mColorState;
    if(mColorProfiles)
        delete mColorProfiles;
}

ColorManager *ColorManager::ColorManagerNew()
{
    if(nullptr == mColorManager)
        mColorManager = new ColorManager();
    return  mColorManager;
}

GDateTime *ColorManager::NightLightGetDateTimeNow()
{
    if(datetime_override != NULL)
        return g_date_time_ref (datetime_override);
    return g_date_time_new_now_local();
}

bool ColorManager::NightLightSmoothCb (ColorManager *manager)
{
        double tmp;
        double frac;

        /* find fraction */
        frac = g_timer_elapsed (manager->smooth_timer, NULL) / USD_NIGHT_LIGHT_SMOOTH_SMEAR;
        if (frac >= 1.f) {
                manager->NightLightSetTemperatureInternal (manager->smooth_target_temperature);
                manager->smooth_id = 0;
                return G_SOURCE_REMOVE;
        }

        /* set new temperature step using log curve */
        tmp = manager->smooth_target_temperature - manager->cached_temperature;
        tmp *= frac;
        tmp += manager->cached_temperature;
        manager->NightLightSetTemperatureInternal (tmp);

        return G_SOURCE_CONTINUE;
}

void ColorManager::PollSmoothCreate (double temperature)
{
        g_assert (smooth_id == 0);
        smooth_target_temperature = temperature;
        smooth_timer = g_timer_new ();
        smooth_id = g_timeout_add (50, (GSourceFunc)NightLightSmoothCb, this);
}

void ColorManager::PollSmoothDestroy ()
{
        if (smooth_id != 0) {
                g_source_remove (smooth_id);
                smooth_id = 0;
        }
        if (smooth_timer != NULL)
                g_clear_pointer (&smooth_timer, g_timer_destroy);
}

void ColorManager::NightLightSetTemperatureInternal (double temperature)
{
        if (ABS (cached_temperature - temperature) <= USD_TEMPERATURE_MAX_DELTA)
                return;
        cached_temperature = temperature;
    mColorState->ColorStateSetTemperature (cached_temperature);
}

void ColorManager::NightLightSetTemperature(double temperature)
{
    /* immediate */
    if (!smooth_enabled) {
            NightLightSetTemperatureInternal (temperature);
            return;
    }

    /* Destroy any smooth transition, it will be recreated if neccessary */
    PollSmoothDestroy ();

    /* small jump */
    if (ABS (temperature - cached_temperature) < USD_TEMPERATURE_MAX_DELTA) {
            NightLightSetTemperatureInternal (temperature);
            return;
    }

    /* smooth out the transition */
    PollSmoothCreate (temperature);
}

void ColorManager::NightLightSetActive(bool active)
{
    cached_active = active;
    /* ensure set to unity temperature */
    if (!active){
        NightLightSetTemperature (USD_COLOR_TEMPERATURE_DEFAULT);
    }
}


static gdouble
deg2rad (gdouble degrees)
{
        return (M_PI * degrees) / 180.f;
}

static gdouble
rad2deg (gdouble radians)
{
        return radians * (180.f / M_PI);
}

/*
 * Formulas taken from https://www.esrl.noaa.gov/gmd/grad/solcalc/calcdetails.html
 *
 * The returned values are fractional hours, so 6am would be 6.0 and 4:30pm
 * would be 16.5.
 *
 * The values returned by this function might not make sense for locations near
 * the polar regions. For example, in the north of Lapland there might not be
 * a sunrise at all.
 */
bool NightLightGetSunriseSunset (GDateTime *dt,
                                double pos_lat,  double pos_long,
                                double *sunrise, double *sunset)
{
        g_autoptr(GDateTime) dt_zero = g_date_time_new_utc (1900, 1, 1, 0, 0, 0);
        GTimeSpan ts = g_date_time_difference (dt, dt_zero);

        g_return_val_if_fail (pos_lat <= 90.f && pos_lat >= -90.f, false);
        g_return_val_if_fail (pos_long <= 180.f && pos_long >= -180.f, false);

        double tz_offset = (double) g_date_time_get_utc_offset (dt) / G_USEC_PER_SEC / 60 / 60; // B5
        double date_as_number = ts / G_USEC_PER_SEC / 24 / 60 / 60 + 2;  // B7
        double time_past_local_midnight = 0;  // E2, unused in this calculation
        double julian_day = date_as_number + 2415018.5 +
                        time_past_local_midnight - tz_offset / 24;
        double julian_century = (julian_day - 2451545) / 36525;
        double geom_mean_long_sun = fmod (280.46646 + julian_century *
                       (36000.76983 + julian_century * 0.0003032), 360); // I2
        double geom_mean_anom_sun = 357.52911 + julian_century *
                        (35999.05029 - 0.0001537 * julian_century);  // J2
        double eccent_earth_orbit = 0.016708634 - julian_century *
                        (0.000042037 + 0.0000001267 * julian_century); // K2
        double sun_eq_of_ctr = sin (deg2rad (geom_mean_anom_sun)) *
                        (1.914602 - julian_century * (0.004817 + 0.000014 * julian_century)) +
                        sin (deg2rad (2 * geom_mean_anom_sun)) * (0.019993 - 0.000101 * julian_century) +
                        sin (deg2rad (3 * geom_mean_anom_sun)) * 0.000289; // L2
        double sun_true_long = geom_mean_long_sun + sun_eq_of_ctr; // M2
        double sun_app_long = sun_true_long - 0.00569 - 0.00478 *
                        sin (deg2rad (125.04 - 1934.136 * julian_century)); // P2
        double mean_obliq_ecliptic = 23 +  (26 +  ((21.448 - julian_century *
                        (46.815 + julian_century * (0.00059 - julian_century * 0.001813)))) / 60) / 60; // Q2
        double obliq_corr = mean_obliq_ecliptic + 0.00256 *
                        cos (deg2rad (125.04 - 1934.136 * julian_century)); // R2
        double sun_declin = rad2deg (asin (sin (deg2rad (obliq_corr)) *
                                            sin (deg2rad (sun_app_long)))); // T2
        double var_y = tan (deg2rad (obliq_corr/2)) * tan (deg2rad (obliq_corr / 2)); // U2
        double eq_of_time = 4 * rad2deg (var_y * sin (2 * deg2rad (geom_mean_long_sun)) -
                        2 * eccent_earth_orbit * sin (deg2rad (geom_mean_anom_sun)) +
                        4 * eccent_earth_orbit * var_y *
                                sin (deg2rad (geom_mean_anom_sun)) *
                                cos (2 * deg2rad (geom_mean_long_sun)) -
                        0.5 * var_y * var_y * sin (4 * deg2rad (geom_mean_long_sun)) -
                        1.25 * eccent_earth_orbit * eccent_earth_orbit *
                                sin (2 * deg2rad (geom_mean_anom_sun))); // V2
        double ha_sunrise = rad2deg (acos (cos (deg2rad (90.833)) / (cos (deg2rad (pos_lat)) *
                        cos (deg2rad (sun_declin))) - tan (deg2rad (pos_lat)) *
                        tan (deg2rad (sun_declin)))); // W2
        double solar_noon =  (720 - 4 * pos_long - eq_of_time + tz_offset * 60) / 1440; // X2
        double sunrise_time = solar_noon - ha_sunrise * 4 / 1440; //  Y2
        double sunset_time = solar_noon + ha_sunrise * 4 / 1440; // Z2

        /* convert to hours */
        if (sunrise != NULL)
                *sunrise = sunrise_time * 24;
        if (sunset != NULL)
                *sunset = sunset_time * 24;
        return true;
}

double NightLightFracDayFromDt (GDateTime *dt)
{
        return g_date_time_get_hour (dt) +
               (double) g_date_time_get_minute (dt) / 60.f +
               (double) g_date_time_get_second (dt) / 3600.f;
}

bool NightLightFracDayIsBetween (double  value,
                                          double  start,
                                          double  end)
{
        /* wrap end to the next day if it is before start,
         * considering equal values as a full 24h period
         */
        if (end <= start)
                end += 24;

        /* wrap value to the next day if it is before the range */
        if (value < start && value < end)
                value += 24;

        /* Check whether value falls into range; together with the 24h
         * wrap around above this means that TRUE is always returned when
         * start == end.
         */
        return value >= start && value < end;
}

static double
LinearInterpolate (double val1, double val2, double factor)
{
        g_return_val_if_fail (factor >= 0.f, -1.f);
        g_return_val_if_fail (factor <= 1.f, -1.f);
        return ((val1 - val2) * factor) + val2;
}

bool ColorManager::UpdateCachedSunriseSunset()
{
    bool ret = false;
    double latitude;
    double longitude;
    double sunrise;
    double sunset;
    g_autoptr(GVariant) tmp = NULL;
    g_autoptr(GDateTime) dt_now = NightLightGetDateTimeNow ();

    GSettings *setting = g_settings_new(PLUGIN_COLOR_SCHEMA);
    /* calculate the sunrise/sunset for the location */
    tmp = g_settings_get_value (setting, COLOR_KEY_LAST_COORDINATES);
    g_clear_object(&setting);

    g_variant_get (tmp, "(dd)", &latitude, &longitude);
    if (latitude > 90.f || latitude < -90.f)
            return false;
    if (longitude > 180.f || longitude < -180.f)
            return false;
    if (!NightLightGetSunriseSunset (dt_now, latitude, longitude,
                                               &sunrise, &sunset)) {
            qWarning ("failed to get sunset/sunrise for %.3f,%.3f",
                       longitude, longitude);
            return false;
    }

    /* anything changed */
    if (ABS (cached_sunrise - sunrise) > USD_FRAC_DAY_MAX_DELTA) {
            cached_sunrise = sunrise;
            ret = true;
    }
    if (ABS (cached_sunset - sunset) > USD_FRAC_DAY_MAX_DELTA) {
        cached_sunset = sunset;
        ret = true;
    }
    return ret;
}

void ColorManager::NightLightRecheck(ColorManager *manager)
{
    double frac_day;
    double schedule_from = -1.f;
    double schedule_to = -1.f;
    double theme_from = -1.f;
    double theme_to = -1.f;
    double smear = USD_NIGHT_LIGHT_POLL_SMEAR; /* hours */
    int theme_now = -1;

    guint temperature;
    guint temp_smeared;
    GDateTime *dt_now = manager->NightLightGetDateTimeNow ();

    /* Forced mode, just set the temperature to night light.
     * Proper rechecking will happen once forced mode is disabled again */
    if (manager->forced) {
        temperature = manager->settings->get(COLOR_KEY_TEMPERATURE).toUInt();
        manager->NightLightSetTemperature (temperature);
        return;
    }

    /* calculate the position of the sun */
    if (manager->settings->keys().contains(COLOR_KEY_AUTO_THEME)) {
        if (manager->settings->get(COLOR_KEY_AUTO_THEME).toBool()) {
            manager->UpdateCachedSunriseSunset ();
            //        if (manager->cached_sunrise > 0.f && manager->cached_sunset > 0.f) {
            //            theme_to   = manager->cached_sunrise;
            //            theme_from = manager->cached_sunset;
            //        } else {
            //            theme_to = 7.0;
            //            theme_from = 18.0;
            //        }
            theme_to = 7.0;
            theme_from = 18.0;
            /* get the current hour of a day as a fraction */
            frac_day = NightLightFracDayFromDt (dt_now);
            //        qDebug ("fractional day = %.3f, limits = %.3f->%.3f",
            //             frac_day, theme_from, theme_to);
            if(frac_day > theme_to && frac_day < theme_from)
                theme_now = 0;
            else
                theme_now = 1;
            if(theme_now){
                manager->gtk_settings->set(GTK_THEME_KEY, "ukui-black-unity");
                manager->qt_settings->set(QT_THEME_KEY, "ukui-dark");
            }
            else{
                manager->gtk_settings->set(GTK_THEME_KEY, "ukui-white-unity");
                manager->qt_settings->set(QT_THEME_KEY, "ukui-light");
            }
        }
    }

    if(!manager->settings->get(COLOR_KEY_ENABLED).toBool()){
//        qDebug("night light disabled, resetting");
        manager->NightLightSetActive (false);
        return;
    }

    if(manager->settings->get(COLOR_KEY_ALLDAY).toBool()){
//        qDebug() << "all day all day all day";
        temperature = manager->settings->get(COLOR_KEY_TEMPERATURE).toUInt();
        manager->NightLightSetTemperature (temperature);
        return;
    }

    /* calculate the position of the sun */
    if (manager->settings->get(COLOR_KEY_AUTOMATIC).toBool()) {
        manager->UpdateCachedSunriseSunset ();
        if (manager->cached_sunrise > 0.f && manager->cached_sunset > 0.f) {
            schedule_to   = manager->cached_sunrise;
            schedule_from = manager->cached_sunset;
            manager->settings->set(COLOR_KEY_AUTOMATIC_FROM, 18.00);
            manager->settings->set(COLOR_KEY_AUTOMATIC_TO, 7.00);
        }
    }

    /* fall back to manual settings */
    if (schedule_to <= 0.f || schedule_from <= 0.f) {
        schedule_from = manager->settings->get(COLOR_KEY_FROM).toDouble();
        schedule_to = manager->settings->get(COLOR_KEY_TO).toDouble();
    }

    /* get the current hour of a day as a fraction */
    frac_day = NightLightFracDayFromDt (dt_now);
//    qDebug ("fractional day = %.3f, limits = %.3f->%.3f",
//         frac_day, schedule_from, schedule_to);

    /* disabled until tomorrow */
    if (manager->disabled_until_tmw) {
        GTimeSpan time_span;
        bool reset = false;

        time_span = g_date_time_difference (dt_now, manager->disabled_until_tmw_dt);

        /* Reset if disabled until tomorrow is more than 24h ago. */
        if (time_span > (GTimeSpan) 24 * 60 * 60 * 1000000) {
            qDebug ("night light disabled until tomorrow is older than 24h, resetting disabled until tomorrow");
            reset = true;
        } else if (time_span > 0) {
            /* Or if a sunrise lies between the time it was disabled and now. */
            gdouble frac_disabled;
            frac_disabled = NightLightFracDayFromDt (manager->disabled_until_tmw_dt);
            if (frac_disabled != frac_day &&
                NightLightFracDayIsBetween (schedule_to,
                                            frac_disabled,
                                            frac_day)) {
                    qDebug ("night light sun rise happened, resetting disabled until tomorrow");
                    reset = true;
            }
        }

        if (reset) {
                manager->disabled_until_tmw = false;
                g_clear_pointer(&manager->disabled_until_tmw_dt, g_date_time_unref);
        } else {
                qDebug ("night light still day-disabled, resetting");
                manager->NightLightSetTemperature (USD_COLOR_TEMPERATURE_DEFAULT);
                return;
        }
    }

    /* lower smearing period to be smaller than the time between start/stop */
    smear = MIN (smear,
                MIN (ABS (schedule_to - schedule_from),
                     24 - ABS (schedule_to - schedule_from)));

    if (!NightLightFracDayIsBetween (frac_day,
                                     schedule_from - smear,
                                     schedule_to)) {
//        qDebug() << "not time for night-light";
        manager->NightLightSetActive (false);
        return;
    }

    /* smear the temperature for a short duration before the set limits
    *
    *   |----------------------| = from->to
    * |-|                        = smear down
    *                        |-| = smear up
    *
    * \                        /
    *  \                      /
    *   \--------------------/
    */
    temperature = manager->settings->get(COLOR_KEY_TEMPERATURE).toUInt();
    if (smear < 0.01) {
        /* Don't try to smear for extremely short or zero periods */
        temp_smeared = temperature;
    } else if (NightLightFracDayIsBetween (frac_day,
                                           schedule_from - smear,
                                           schedule_from)) {
        double factor = 1.f - ((frac_day - (schedule_from - smear)) / smear);
        temp_smeared = LinearInterpolate (USD_COLOR_TEMPERATURE_DEFAULT,
                                          temperature, factor);
    } else if (NightLightFracDayIsBetween (frac_day,
                                           schedule_to - smear,
                                           schedule_to)) {
        double factor = (frac_day - (schedule_to - smear)) / smear;
        temp_smeared = LinearInterpolate (USD_COLOR_TEMPERATURE_DEFAULT,
                                          temperature, factor);
    } else {
        temp_smeared = temperature;
    }
//    qDebug ("night light mode on, using temperature of %uK (aiming for %uK)",
//         temp_smeared, temperature);
    manager->NightLightSetActive (true);
    manager->NightLightSetTemperature (temp_smeared);
}

void ColorManager::OnLocationNotify(GClueSimple *simple,
                                    GParamSpec *pspec,
                                    gpointer user_data)
{
    GClueLocation *location;
    gdouble latitude, longitude;
    ColorManager *manager = (ColorManager *)user_data;
    location = gclue_simple_get_location (simple);
    latitude = gclue_location_get_latitude (location);
    longitude = gclue_location_get_longitude (location);
    GSettings *setting = g_settings_new(PLUGIN_COLOR_SCHEMA);
    /* calculate the sunrise/sunset for the location */
    g_settings_set_value (setting,
                          COLOR_KEY_LAST_COORDINATES,
                          g_variant_new ("(dd)", latitude, longitude));
    g_clear_object(&setting);

    // qDebug ("got geoclue latitude %f, longitude %f", latitude, longitude);

    /* recheck the levels if the location changed significantly */
    if (manager->UpdateCachedSunriseSunset ())
           manager->NightLightRecheck (manager);
}
void ColorManager::OnGeoclueSimpleReady(GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
    GClueSimple *geoclue_simple;
    ColorManager *manager = (ColorManager *)user_data;
    g_autoptr(GError) error = NULL;

    geoclue_simple = gclue_simple_new_finish (res, &error);
    if (geoclue_simple == NULL) {
            if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                    qWarning ("Failed to connect to GeoClue2 service: %s", error->message);
            return;
    }

    manager->geoclue_simple = geoclue_simple;
    manager->geoclue_client = gclue_simple_get_client (manager->geoclue_simple);
    g_object_set (G_OBJECT (manager->geoclue_client),
                  "time-threshold", 60*60, NULL); /* 1 hour */

    g_signal_connect (manager->geoclue_simple, "notify::location",
                      G_CALLBACK (OnLocationNotify), user_data);

    OnLocationNotify (manager->geoclue_simple, NULL, user_data);
}

void ColorManager::StartGeoclue()
{
    cancellable = g_cancellable_new ();
    gclue_simple_new (DESKTOP_ID,
                      GCLUE_ACCURACY_LEVEL_CITY,
                      cancellable,
                      OnGeoclueSimpleReady,
                      this);
}

void ColorManager::StopGeoclue()
{
    g_cancellable_cancel (cancellable);
    g_clear_object (&cancellable);

    if (geoclue_client != NULL) {
            gclue_client_call_stop (geoclue_client, NULL, NULL, NULL);
            geoclue_client = NULL;
    }
    g_clear_object (&geoclue_simple);
}

void ColorManager::SettingsChangedCb(QString key)
{
//    qDebug ("settings changed");
    if(key == COLOR_KEY_AUTOMATIC_FROM || key == COLOR_KEY_AUTOMATIC_TO){
        return;
    }
    NightLightRecheck(this);
    mColorState->ColorStateSetTemperature (cached_temperature);
}

bool ColorManager::ColorManagerStart()
{
    qDebug()<<"Color manager start";
    mColorProfiles->ColorProfilesStart();
    mColorState->ColorStateStart();
    NightLightRecheck(this);

    QTimer * timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkTime()));
    timer->start(USD_NIGHT_LIGHT_POLL_TIMEOUT*1000);

    StartGeoclue();
    connect(settings, SIGNAL(changed(QString)), this, SLOT(SettingsChangedCb(QString)));
    return true;
}

void ColorManager::checkTime()
{
    NightLightRecheck (this);
}

void ColorManager::ColorManagerStop()
{
    qDebug()<<"Color manager stop";
    mColorProfiles->ColorProfilesStop();
    mColorState->ColorStateStop();
    StopGeoclue();
}

