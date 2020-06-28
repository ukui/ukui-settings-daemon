/* usdtimeline.c
 *
 * Copyright (C) 2008 Carlos Garnacho  <carlos@imendio.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __USD_TIMELINE_H__
#define __USD_TIMELINE_H__

#include <glib-object.h>
#include <gdk/gdk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_TIMELINE_DIRECTION       (usd_timeline_direction_get_type ())
#define USD_TYPE_TIMELINE_PROGRESS_TYPE   (usd_timeline_progress_type_get_type ())
#define USD_TYPE_TIMELINE                 (usd_timeline_get_type ())
#define USD_TIMELINE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), USD_TYPE_TIMELINE, UsdTimeline))
#define USD_TIMELINE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  USD_TYPE_TIMELINE, UsdTimelineClass))
#define USD_IS_TIMELINE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), USD_TYPE_TIMELINE))
#define USD_IS_TIMELINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  USD_TYPE_TIMELINE))
#define USD_TIMELINE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  USD_TYPE_TIMELINE, UsdTimelineClass))

typedef enum {
  USD_TIMELINE_DIRECTION_FORWARD,
  USD_TIMELINE_DIRECTION_BACKWARD
} UsdTimelineDirection;

typedef enum {
  USD_TIMELINE_PROGRESS_LINEAR,
  USD_TIMELINE_PROGRESS_SINUSOIDAL,
  USD_TIMELINE_PROGRESS_EXPONENTIAL
} UsdTimelineProgressType;

typedef struct UsdTimeline      UsdTimeline;
typedef struct UsdTimelineClass UsdTimelineClass;

struct UsdTimeline
{
  GObject parent_instance;
};

struct UsdTimelineClass
{
  GObjectClass parent_class;

  void (* started)           (UsdTimeline *timeline);
  void (* finished)          (UsdTimeline *timeline);
  void (* paused)            (UsdTimeline *timeline);

  void (* frame)             (UsdTimeline *timeline,
			      gdouble      progress);

  void (* __usd_reserved1) (void);
  void (* __usd_reserved2) (void);
  void (* __usd_reserved3) (void);
  void (* __usd_reserved4) (void);
};

typedef gdouble (*UsdTimelineProgressFunc) (gdouble progress);


GType                   usd_timeline_get_type           (void) G_GNUC_CONST;
GType                   usd_timeline_direction_get_type (void) G_GNUC_CONST;
GType                   usd_timeline_progress_type_get_type (void) G_GNUC_CONST;

UsdTimeline            *usd_timeline_new                (guint                    duration);
UsdTimeline            *usd_timeline_new_for_screen     (guint                    duration,
							 GdkScreen               *screen);

void                    usd_timeline_start              (UsdTimeline             *timeline);
void                    usd_timeline_pause              (UsdTimeline             *timeline);
void                    usd_timeline_rewind             (UsdTimeline             *timeline);

gboolean                usd_timeline_is_running         (UsdTimeline             *timeline);

guint                   usd_timeline_get_fps            (UsdTimeline             *timeline);
void                    usd_timeline_set_fps            (UsdTimeline             *timeline,
							 guint                    fps);

gboolean                usd_timeline_get_loop           (UsdTimeline             *timeline);
void                    usd_timeline_set_loop           (UsdTimeline             *timeline,
							 gboolean                 loop);

guint                   usd_timeline_get_duration       (UsdTimeline             *timeline);
void                    usd_timeline_set_duration       (UsdTimeline             *timeline,
							 guint                    duration);

GdkScreen              *usd_timeline_get_screen         (UsdTimeline             *timeline);
void                    usd_timeline_set_screen         (UsdTimeline             *timeline,
							 GdkScreen               *screen);

UsdTimelineDirection    usd_timeline_get_direction      (UsdTimeline             *timeline);
void                    usd_timeline_set_direction      (UsdTimeline             *timeline,
							 UsdTimelineDirection     direction);

UsdTimelineProgressType usd_timeline_get_progress_type  (UsdTimeline             *timeline);
void                    usd_timeline_set_progress_type  (UsdTimeline             *timeline,
							 UsdTimelineProgressType  type);
void                    usd_timeline_get_progress_func  (UsdTimeline             *timeline);

void                    usd_timeline_set_progress_func  (UsdTimeline             *timeline,
							 UsdTimelineProgressFunc  progress_func);

gdouble                 usd_timeline_get_progress       (UsdTimeline             *timeline);


#ifdef __cplusplus
}
#endif

#endif /* __USD_TIMELINE_H__ */
