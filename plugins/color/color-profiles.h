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
#ifndef COLORPROFILES_H
#define COLORPROFILES_H

#include <QObject>
#include "config.h"
#include <stdio.h>
#include <glib/gi18n.h>
#include <colord.h>

class ColorProfiles : public QObject
{
    Q_OBJECT

public:
    ColorProfiles();
    ColorProfiles(ColorProfiles&)=delete;
    ~ColorProfiles();

    bool ColorProfilesStart();
    void ColorProfilesStop();

public:
    static void SessionIccStoreAddedCb (CdIccStore *icc_store,
                                        CdIcc *icc,
                                        ColorProfiles *profiles);
    static void SessionIccStoreRemovedCb (CdIccStore *icc_store,
                                          CdIcc *icc,
                                          ColorProfiles *profiles);
    static void SessionFindProfileByFilenameCb (GObject *object,
                                                GAsyncResult *res,
                                                gpointer user_data);
    static void SessionClientConnectCb (GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data);

private:
    GCancellable    *cancellable;
    CdClient        *client;
    CdIccStore      *icc_store;

};

#endif // COLORPROFILES_H
