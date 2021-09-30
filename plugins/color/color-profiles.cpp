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
#include "color-profiles.h"

static void
SessionCreateProfileCb (GObject *object,
                        GAsyncResult *res,
                        gpointer user_data)
{
        CdProfile *profile;
        GError *error = NULL;
        CdClient *client = CD_CLIENT (object);

        profile = cd_client_create_profile_finish (client, res, &error);
        if (profile == NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
                    !g_error_matches (error, CD_CLIENT_ERROR, CD_CLIENT_ERROR_ALREADY_EXISTS))
                        g_warning ("%s", error->message);
                g_error_free (error);
                return;
        }
        g_object_unref (profile);
}

static void
SessionDeleteProfileCb (GObject *object,
                        GAsyncResult *res,
                        gpointer user_data)
{
        gboolean ret;
        GError *error = NULL;
        CdClient *client = CD_CLIENT (object);

        ret = cd_client_delete_profile_finish (client, res, &error);
        if (!ret) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                        g_warning ("%s", error->message);
                g_error_free (error);
        }
}

void ColorProfiles::SessionFindProfileByFilenameCb (GObject *object,
                                                    GAsyncResult *res,
                                                    gpointer user_data)
{
        GError *error = NULL;
        CdProfile *profile;
        CdClient *client = CD_CLIENT (object);
        ColorProfiles *profiles = (ColorProfiles *)user_data;

        profile = cd_client_find_profile_by_filename_finish (client, res, &error);
        if (profile == NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                        g_warning ("%s", error->message);
                g_error_free (error);
                goto out;
        }

        /* remove it from colord */
        cd_client_delete_profile (profiles->client,
                                  profile,
                                  profiles->cancellable,
                                  SessionDeleteProfileCb,
                                  profiles);
out:
        if (profile != NULL)
                g_object_unref (profile);
}

void ColorProfiles::SessionIccStoreAddedCb (CdIccStore *icc_store,
                                            CdIcc *icc,
                                            ColorProfiles *profiles)
{
        cd_client_create_profile_for_icc (profiles->client,
                                          icc,
                                          CD_OBJECT_SCOPE_TEMP,
                                          profiles->cancellable,
                                          SessionCreateProfileCb,
                                          profiles);
}

void ColorProfiles::SessionIccStoreRemovedCb (CdIccStore *icc_store,
                                              CdIcc *icc,
                                              ColorProfiles *profiles)
{
        /* find the ID for the filename */
//        qDebug ("filename %s removed", cd_icc_get_filename (icc));
        cd_client_find_profile_by_filename (profiles->client,
                                            cd_icc_get_filename (icc),
                                            profiles->cancellable,
                                            SessionFindProfileByFilenameCb,
                                            profiles);
}

ColorProfiles::ColorProfiles()
{
    cancellable = nullptr;
    client = cd_client_new ();
    icc_store = cd_icc_store_new ();
    cd_icc_store_set_load_flags (icc_store,
                                 CD_ICC_LOAD_FLAGS_FALLBACK_MD5);
    g_signal_connect (icc_store, "added",
                      G_CALLBACK (SessionIccStoreAddedCb),
                      this);
    g_signal_connect (icc_store, "removed",
                      G_CALLBACK (SessionIccStoreRemovedCb),
                      this);
}

ColorProfiles::~ColorProfiles()
{
    g_cancellable_cancel (cancellable);
    g_clear_object (&cancellable);
    g_clear_object (&icc_store);
    g_clear_object (&client);
}

void ColorProfiles::SessionClientConnectCb (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
        gboolean ret;
        GError *error = NULL;
        CdClient *client = CD_CLIENT (source_object);
        ColorProfiles *profiles;

        /* connected */
        ret = cd_client_connect_finish (client, res, &error);

        if (!ret) {
            if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                USD_LOG(LOG_DEBUG,"failed to connect to colord: %s", error->message);
            }
            g_error_free (error);
            return;
        }

        /* is there an available colord instance? */
        profiles = (ColorProfiles *)user_data;
        ret = cd_client_get_has_server (profiles->client);

        if (!ret) {
            USD_LOG(LOG_DEBUG,"There is no colord server available");
            return;
        }

        /* add profiles */
        ret = cd_icc_store_search_kind (profiles->icc_store,
                                        CD_ICC_STORE_SEARCH_KIND_USER,
                                        CD_ICC_STORE_SEARCH_FLAGS_CREATE_LOCATION,
                                        profiles->cancellable,
                                        &error);
        if (!ret) {
                g_error_free (error);
                USD_LOG(LOG_DEBUG,"failed to add user icc: %s", error->message);
        }
        USD_LOG(LOG_DEBUG,"SessionClientConnectCb over..");
}

bool ColorProfiles::ColorProfilesStart()
{
    /* use a fresh cancellable for each start->stop operation */
    g_cancellable_cancel (cancellable);
    g_clear_object (&cancellable);
    cancellable = g_cancellable_new ();

    cd_client_connect (client,
                       cancellable,
                       SessionClientConnectCb,
                       this);

    return true;
}

void ColorProfiles::ColorProfilesStop()
{
    g_cancellable_cancel (cancellable);
}
