/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * usd-ldsm-dialog.c
 * Copyright (C) Chris Coulson 2009 <chrisccoulson@googlemail.com>
 *
 * usd-ldsm-dialog.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * usd-ldsm-dialog.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _USD_LDSM_DIALOG_H_
#define _USD_LDSM_DIALOG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_LDSM_DIALOG             (usd_ldsm_dialog_get_type ())
#define USD_LDSM_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), USD_TYPE_LDSM_DIALOG, UsdLdsmDialog))
#define USD_LDSM_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), USD_TYPE_LDSM_DIALOG, UsdLdsmDialogClass))
#define USD_IS_LDSM_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), USD_TYPE_LDSM_DIALOG))
#define USD_IS_LDSM_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), USD_TYPE_LDSM_DIALOG))
#define USD_LDSM_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), USD_TYPE_LDSM_DIALOG, UsdLdsmDialogClass))

enum
{
        USD_LDSM_DIALOG_RESPONSE_EMPTY_TRASH = -20,
        USD_LDSM_DIALOG_RESPONSE_ANALYZE = -21
};

typedef struct UsdLdsmDialogPrivate UsdLdsmDialogPrivate;
typedef struct _UsdLdsmDialogClass UsdLdsmDialogClass;
typedef struct _UsdLdsmDialog UsdLdsmDialog;

struct _UsdLdsmDialogClass
{
        GtkDialogClass parent_class;
};

struct _UsdLdsmDialog
{
        GtkDialog parent_instance;
        UsdLdsmDialogPrivate *priv;
};

GType usd_ldsm_dialog_get_type (void) G_GNUC_CONST;

UsdLdsmDialog * usd_ldsm_dialog_new (gboolean other_usable_partitions,
                                     gboolean other_partitions,
                                     gboolean display_baobab,
                                     gboolean display_empty_trash,
                                     gint64 space_remaining,
                                     const gchar *partition_name,
                                     const gchar *mount_path);

#ifdef __cplusplus
}
#endif

#endif /* _USD_LDSM_DIALOG_H_ */
