/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "ukui-settings-profile.h"
#include "usd-dummy-manager.h"

#define USD_DUMMY_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_DUMMY_MANAGER, UsdDummyManagerPrivate))

struct UsdDummyManagerPrivate
{
        gboolean padding;
};

enum {
        PROP_0,
};

static void     usd_dummy_manager_class_init  (UsdDummyManagerClass *klass);
static void     usd_dummy_manager_init        (UsdDummyManager      *dummy_manager);
static void     usd_dummy_manager_finalize    (GObject             *object);

G_DEFINE_TYPE (UsdDummyManager, usd_dummy_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

gboolean
usd_dummy_manager_start (UsdDummyManager *manager,
                               GError               **error)
{
        g_debug ("Starting dummy manager");
        ukui_settings_profile_start (NULL);
        ukui_settings_profile_end (NULL);
        return TRUE;
}

void
usd_dummy_manager_stop (UsdDummyManager *manager)
{
        g_debug ("Stopping dummy manager");
}

static void
usd_dummy_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
usd_dummy_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
usd_dummy_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        UsdDummyManager      *dummy_manager;

        dummy_manager = USD_DUMMY_MANAGER (G_OBJECT_CLASS (usd_dummy_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (dummy_manager);
}

static void
usd_dummy_manager_dispose (GObject *object)
{
        G_OBJECT_CLASS (usd_dummy_manager_parent_class)->dispose (object);
}

static void
usd_dummy_manager_class_init (UsdDummyManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = usd_dummy_manager_get_property;
        object_class->set_property = usd_dummy_manager_set_property;
        object_class->constructor = usd_dummy_manager_constructor;
        object_class->dispose = usd_dummy_manager_dispose;
        object_class->finalize = usd_dummy_manager_finalize;

        g_type_class_add_private (klass, sizeof (UsdDummyManagerPrivate));
}

static void
usd_dummy_manager_init (UsdDummyManager *manager)
{
        manager->priv = USD_DUMMY_MANAGER_GET_PRIVATE (manager);

}

static void
usd_dummy_manager_finalize (GObject *object)
{
        UsdDummyManager *dummy_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_DUMMY_MANAGER (object));

        dummy_manager = USD_DUMMY_MANAGER (object);

        g_return_if_fail (dummy_manager->priv != NULL);

        G_OBJECT_CLASS (usd_dummy_manager_parent_class)->finalize (object);
}

UsdDummyManager *
usd_dummy_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (USD_TYPE_DUMMY_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return USD_DUMMY_MANAGER (manager_object);
}
