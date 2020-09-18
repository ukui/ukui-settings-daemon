#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QObject>
#include <QGSettings/qgsettings.h>

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

public Q_SLOTS:
    void SettingsChangedCb(QString);

public:
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
    static bool NightLightRecheckCb(ColorManager *manager);
    void NightLightSetActive(bool active);
    bool UpdateCachedSunriseSunset();
    static void PollTimeoutCreate(ColorManager *manager);
    static void PollTimeoutDestroy(ColorManager *manager);

private:
    static ColorManager *mColorManager;
    ColorProfiles       *mColorProfiles;
    ColorState          *mColorState;

    QGSettings *settings;
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
    guint       smooth_id;
    double      smooth_target_temperature;
    GCancellable  *cancellable;
    GClueClient   *geoclue_client;
    GClueSimple   *geoclue_simple;
};

#endif // COLORMANAGER_H
