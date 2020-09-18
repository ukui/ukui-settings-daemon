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
