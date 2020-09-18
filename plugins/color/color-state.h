#ifndef COLORSTATE_H
#define COLORSTATE_H

#include <QObject>

#include <glib/gi18n.h>
#include <colord.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <lcms2.h>
#include <canberra-gtk.h>

#include "color-edid.h"

extern "C" {
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr.h>
}

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

class ColorState : public QObject
{
    Q_OBJECT

public:
    ColorState();
    ColorState(ColorState&)=delete;
    ~ColorState();

    bool ColorStateStart();
    void ColorStateStop();

public:
    void ColorStateSetTemperature(guint temperature);
    static bool GetSystemIccProfile (ColorState *state,
                                     GFile *file);

    static gchar *SessionGetOutputId (ColorState *state,
                                      MateRROutput *output);
    static ColorEdid *SessionGetOutputEdid (ColorState *state,
                                            MateRROutput *output);
    static bool SessionUseOutputProfileForScreen (ColorState *state,
                                                  MateRROutput *output);
    static bool SessionScreenSetIccProfile (ColorState *state,
                                            const gchar *filename,
                                            GError **error);
    static MateRROutput *SessionGetStateOutputById (ColorState *state,
                                                    const gchar *device_id);
    static gboolean SessionCheckProfileDeviceMd (GFile *file);
    static bool ApplyCreateIccProfileForEdid (ColorState *state,
                                              CdDevice *device,
                                              ColorEdid *edid,
                                              GFile *file,
                                              GError **error);

    static bool SessionDeviceResetGamma (MateRROutput *output,
                                         guint color_temperature);
    static void SessionDeviceAssignProfileConnectCb (GObject *object,
                                                     GAsyncResult *res,
                                                     gpointer user_data);
    static void SessionAddStateOutput (ColorState *state, MateRROutput *output);
    static void SessionProfileGammaFindDeviceCb (GObject *object,
                                                 GAsyncResult *res,
                                                 gpointer user_data);
    static void SessionDeviceAssignConnectCb (GObject *object,
                                              GAsyncResult *res,
                                              gpointer user_data);
    static void SessionClientConnectCb (GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data);
    static void SessionSetGammaForAllDevices (ColorState *state);
    static void SessionGetDevicesCb (GObject *object, GAsyncResult *res, gpointer user_data);
    static void SessionDeviceAssign (ColorState *state, CdDevice *device);
    static void SessionDeviceChangedAssignCb (CdClient *client,
                                              CdDevice *device,
                                              ColorState *stata);
    static void SessionDeviceAddedAssignCb (CdClient *client,
                                            CdDevice *device,
                                            ColorState *state);
    static void MateRrScreenOutputChangedCb (MateRRScreen *screen,
                                             ColorState *state);

private:
    GCancellable    *cancellable;
    CdClient        *client;
    MateRRScreen    *state_screen;
    GHashTable      *edid_cache;
    GdkWindow       *gdk_window;
    GHashTable      *device_assign_hash;
    guint            color_temperature;

};

#endif // COLORSTATE_H
