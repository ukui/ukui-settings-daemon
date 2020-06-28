/* usd-timeline.c
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

#include <glib.h>
#include <gtk/gtk.h>
#include <math.h>
#include "usd-timeline.h"

#define USD_TIMELINE_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), USD_TYPE_TIMELINE, UsdTimelinePriv))
#define MSECS_PER_SEC 1000
#define FRAME_INTERVAL(nframes) (MSECS_PER_SEC / nframes)
#define DEFAULT_FPS 30

typedef struct UsdTimelinePriv UsdTimelinePriv;

struct UsdTimelinePriv
{
  guint duration;
  guint fps;
  guint source_id;

  GTimer *timer;

  GdkScreen *screen;
  UsdTimelineProgressType progress_type;
  UsdTimelineProgressFunc progress_func;

  guint loop      : 1;
  guint direction : 1;
};

enum {
  PROP_0,
  PROP_FPS,
  PROP_DURATION,
  PROP_LOOP,
  PROP_DIRECTION,
  PROP_SCREEN,
  PROP_PROGRESS_TYPE,
};

enum {
  STARTED,
  PAUSED,
  FINISHED,
  FRAME,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };


static void  usd_timeline_set_property  (GObject         *object,
					 guint            prop_id,
					 const GValue    *value,
					 GParamSpec      *pspec);
static void  usd_timeline_get_property  (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec);
static void  usd_timeline_finalize      (GObject *object);


G_DEFINE_TYPE (UsdTimeline, usd_timeline, G_TYPE_OBJECT)


GType
usd_timeline_direction_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      static const GEnumValue values[] = {
	{ USD_TIMELINE_DIRECTION_FORWARD,  "USD_TIMELINE_DIRECTION_FORWARD",  "forward" },
	{ USD_TIMELINE_DIRECTION_BACKWARD, "USD_TIMELINE_DIRECTION_BACKWARD", "backward" },
	{ 0, NULL, NULL }
      };

      type = g_enum_register_static (g_intern_static_string ("UsdTimelineDirection"), values);
    }

  return type;
}

GType
usd_timeline_progress_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      static const GEnumValue values[] = {
	{ USD_TIMELINE_PROGRESS_LINEAR,      "USD_TIMELINE_PROGRESS_LINEAR",      "linear" },
	{ USD_TIMELINE_PROGRESS_SINUSOIDAL,  "USD_TIMELINE_PROGRESS_SINUSOIDAL",  "sinusoidal" },
	{ USD_TIMELINE_PROGRESS_EXPONENTIAL, "USD_TIMELINE_PROGRESS_EXPONENTIAL", "exponential" },
	{ 0, NULL, NULL }
      };

      type = g_enum_register_static (g_intern_static_string ("UsdTimelineProgressType"), values);
    }

  return type;
}

static void
usd_timeline_class_init (UsdTimelineClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = usd_timeline_set_property;
  object_class->get_property = usd_timeline_get_property;
  object_class->finalize = usd_timeline_finalize;

  g_object_class_install_property (object_class,
				   PROP_FPS,
				   g_param_spec_uint ("fps",
						      "FPS",
						      "Frames per second for the timeline",
						      1,
						      G_MAXUINT,
						      DEFAULT_FPS,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DURATION,
				   g_param_spec_uint ("duration",
						      "Animation Duration",
						      "Animation Duration",
						      0,
						      G_MAXUINT,
						      0,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_LOOP,
				   g_param_spec_boolean ("loop",
							 "Loop",
							 "Whether the timeline loops or not",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DIRECTION,
				   g_param_spec_enum ("direction",
						      "Direction",
						      "Whether the timeline moves forward or backward in time",
						      USD_TYPE_TIMELINE_DIRECTION,
						      USD_TIMELINE_DIRECTION_FORWARD,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DIRECTION,
				   g_param_spec_enum ("progress-type",
						      "Progress type",
						      "Type of progress through the timeline",
						      USD_TYPE_TIMELINE_PROGRESS_TYPE,
						      USD_TIMELINE_PROGRESS_LINEAR,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
							"Screen",
							"Screen to get the settings from",
							GDK_TYPE_SCREEN,
							G_PARAM_READWRITE));

  signals[STARTED] =
    g_signal_new ("started",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (UsdTimelineClass, started),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[PAUSED] =
    g_signal_new ("paused",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (UsdTimelineClass, paused),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[FINISHED] =
    g_signal_new ("finished",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (UsdTimelineClass, finished),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[FRAME] =
    g_signal_new ("frame",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (UsdTimelineClass, frame),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1,
		  G_TYPE_DOUBLE);

  g_type_class_add_private (class, sizeof (UsdTimelinePriv));
}

static void
usd_timeline_init (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  priv = USD_TIMELINE_GET_PRIV (timeline);

  priv->fps = DEFAULT_FPS;
  priv->duration = 0;
  priv->direction = USD_TIMELINE_DIRECTION_FORWARD;
  priv->screen = gdk_screen_get_default ();
}

static void
usd_timeline_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  UsdTimeline *timeline;

  timeline = USD_TIMELINE (object);

  switch (prop_id)
    {
    case PROP_FPS:
      usd_timeline_set_fps (timeline, g_value_get_uint (value));
      break;
    case PROP_DURATION:
      usd_timeline_set_duration (timeline, g_value_get_uint (value));
      break;
    case PROP_LOOP:
      usd_timeline_set_loop (timeline, g_value_get_boolean (value));
      break;
    case PROP_DIRECTION:
      usd_timeline_set_direction (timeline, g_value_get_enum (value));
      break;
    case PROP_SCREEN:
      usd_timeline_set_screen (timeline,
			       GDK_SCREEN (g_value_get_object (value)));
      break;
    case PROP_PROGRESS_TYPE:
      usd_timeline_set_progress_type (timeline, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
usd_timeline_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  UsdTimeline *timeline;
  UsdTimelinePriv *priv;

  timeline = USD_TIMELINE (object);
  priv = USD_TIMELINE_GET_PRIV (timeline);

  switch (prop_id)
    {
    case PROP_FPS:
      g_value_set_uint (value, priv->fps);
      break;
    case PROP_DURATION:
      g_value_set_uint (value, priv->duration);
      break;
    case PROP_LOOP:
      g_value_set_boolean (value, priv->loop);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, priv->direction);
      break;
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    case PROP_PROGRESS_TYPE:
      g_value_set_enum (value, priv->progress_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
usd_timeline_finalize (GObject *object)
{
  UsdTimelinePriv *priv;

  priv = USD_TIMELINE_GET_PRIV (object);

  if (priv->source_id)
    {
      g_source_remove (priv->source_id);
      priv->source_id = 0;
    }

  if (priv->timer)
      g_timer_destroy (priv->timer);

  G_OBJECT_CLASS (usd_timeline_parent_class)->finalize (object);
}

/* Sinusoidal progress */
static gdouble
sinusoidal_progress (gdouble progress)
{
  return (sinf ((progress * G_PI) / 2));
}

static gdouble
exponential_progress (gdouble progress)
{
  return progress * progress;
}

static UsdTimelineProgressFunc
progress_type_to_func (UsdTimelineProgressType type)
{
  if (type == USD_TIMELINE_PROGRESS_SINUSOIDAL)
    return sinusoidal_progress;
  else if (type == USD_TIMELINE_PROGRESS_EXPONENTIAL)
    return exponential_progress;

  return NULL;
}

static gboolean
usd_timeline_run_frame (UsdTimeline *timeline,
			gboolean     enable_animations)
{
  UsdTimelinePriv *priv;
  gdouble linear_progress, progress;
  guint elapsed_time;
  UsdTimelineProgressFunc progress_func = NULL;

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (enable_animations)
    {
      elapsed_time = (guint) (g_timer_elapsed (priv->timer, NULL) * 1000);

      linear_progress = (gdouble) elapsed_time / priv->duration;

      if (priv->direction == USD_TIMELINE_DIRECTION_BACKWARD)
	linear_progress = 1 - linear_progress;

      linear_progress = CLAMP (linear_progress, 0., 1.);

      if (priv->progress_func)
	progress_func = priv->progress_func;
      else if (priv->progress_type)
	progress_func = progress_type_to_func (priv->progress_type);

      if (progress_func)
	progress = (progress_func) (linear_progress);
      else
	progress = linear_progress;
    }
  else
    progress = (priv->direction == USD_TIMELINE_DIRECTION_FORWARD) ? 1.0 : 0.0;

  g_signal_emit (timeline, signals [FRAME], 0,
		 CLAMP (progress, 0.0, 1.0));

  if ((priv->direction == USD_TIMELINE_DIRECTION_FORWARD && progress >= 1.0) ||
      (priv->direction == USD_TIMELINE_DIRECTION_BACKWARD && progress <= 0.0))
    {
      if (!priv->loop)
	{
	  if (priv->source_id)
	    {
	      g_source_remove (priv->source_id);
	      priv->source_id = 0;
	    }

	  g_signal_emit (timeline, signals [FINISHED], 0);
	  return FALSE;
	}
      else
	usd_timeline_rewind (timeline);
    }

  return TRUE;
}

static gboolean
usd_timeline_frame_idle_func (UsdTimeline *timeline)
{
  return usd_timeline_run_frame (timeline, TRUE);
}

/**
 * usd_timeline_new:
 * @duration: duration in milliseconds for the timeline
 *
 * Creates a new #UsdTimeline with the specified number of frames.
 *
 * Return Value: the newly created #UsdTimeline
 **/
UsdTimeline *
usd_timeline_new (guint duration)
{
  return g_object_new (USD_TYPE_TIMELINE,
		       "duration", duration,
		       NULL);
}

UsdTimeline *
usd_timeline_new_for_screen (guint      duration,
			     GdkScreen *screen)
{
  return g_object_new (USD_TYPE_TIMELINE,
		       "duration", duration,
		       "screen", screen,
		       NULL);
}

/**
 * usd_timeline_start:
 * @timeline: A #UsdTimeline
 *
 * Runs the timeline from the current frame.
 **/
void
usd_timeline_start (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;
  GtkSettings *settings;
  gboolean enable_animations = FALSE;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (priv->screen)
    {
      settings = gtk_settings_get_for_screen (priv->screen);
      g_object_get (settings, "gtk-enable-animations", &enable_animations, NULL);
    }

  if (enable_animations)
    {
      if (!priv->source_id)
	{
	  if (priv->timer)
	    g_timer_continue (priv->timer);
	  else
	    priv->timer = g_timer_new ();

	  /* sanity check */
	  g_assert (priv->fps > 0);

	  g_signal_emit (timeline, signals [STARTED], 0);

	  priv->source_id = gdk_threads_add_timeout (FRAME_INTERVAL (priv->fps),
						     (GSourceFunc) usd_timeline_frame_idle_func,
						     timeline);
	}
    }
  else
    {
      /* If animations are not enabled, only run the last frame,
       * it take us instantaneously to the last state of the animation.
       * The only potential flaw happens when people use the ::finished
       * signal to trigger another animation, or even worse, finally
       * loop into this animation again.
       */
      g_signal_emit (timeline, signals [STARTED], 0);
      usd_timeline_run_frame (timeline, FALSE);
    }
}

/**
 * usd_timeline_pause:
 * @timeline: A #UsdTimeline
 *
 * Pauses the timeline.
 **/
void
usd_timeline_pause (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (priv->source_id)
    {
      g_source_remove (priv->source_id);
      priv->source_id = 0;
      g_timer_stop (priv->timer);
      g_signal_emit (timeline, signals [PAUSED], 0);
    }
}

/**
 * usd_timeline_rewind:
 * @timeline: A #UsdTimeline
 *
 * Rewinds the timeline.
 **/
void
usd_timeline_rewind (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  /* destroy and re-create timer if neccesary  */
  if (priv->timer)
    {
      g_timer_destroy (priv->timer);

      if (usd_timeline_is_running (timeline))
	priv->timer = g_timer_new ();
      else
	priv->timer = NULL;
    }
}

/**
 * usd_timeline_is_running:
 * @timeline: A #UsdTimeline
 *
 * Returns whether the timeline is running or not.
 *
 * Return Value: %TRUE if the timeline is running
 **/
gboolean
usd_timeline_is_running (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), FALSE);

  priv = USD_TIMELINE_GET_PRIV (timeline);

  return (priv->source_id != 0);
}

/**
 * usd_timeline_get_fps:
 * @timeline: A #UsdTimeline
 *
 * Returns the number of frames per second.
 *
 * Return Value: frames per second
 **/
guint
usd_timeline_get_fps (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), 1);

  priv = USD_TIMELINE_GET_PRIV (timeline);
  return priv->fps;
}

/**
 * usd_timeline_set_fps:
 * @timeline: A #UsdTimeline
 * @fps: frames per second
 *
 * Sets the number of frames per second that
 * the timeline will play.
 **/
void
usd_timeline_set_fps (UsdTimeline *timeline,
		      guint        fps)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));
  g_return_if_fail (fps > 0);

  priv = USD_TIMELINE_GET_PRIV (timeline);

  priv->fps = fps;

  if (usd_timeline_is_running (timeline))
    {
      g_source_remove (priv->source_id);
      priv->source_id = gdk_threads_add_timeout (FRAME_INTERVAL (priv->fps),
						 (GSourceFunc) usd_timeline_run_frame,
						 timeline);
    }

  g_object_notify (G_OBJECT (timeline), "fps");
}

/**
 * usd_timeline_get_loop:
 * @timeline: A #UsdTimeline
 *
 * Returns whether the timeline loops to the
 * beginning when it has reached the end.
 *
 * Return Value: %TRUE if the timeline loops
 **/
gboolean
usd_timeline_get_loop (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), FALSE);

  priv = USD_TIMELINE_GET_PRIV (timeline);
  return priv->loop;
}

/**
 * usd_timeline_set_loop:
 * @timeline: A #UsdTimeline
 * @loop: %TRUE to make the timeline loop
 *
 * Sets whether the timeline loops to the beginning
 * when it has reached the end.
 **/
void
usd_timeline_set_loop (UsdTimeline *timeline,
		       gboolean     loop)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);
  priv->loop = loop;

  g_object_notify (G_OBJECT (timeline), "loop");
}

void
usd_timeline_set_duration (UsdTimeline *timeline,
			   guint        duration)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  priv->duration = duration;

  g_object_notify (G_OBJECT (timeline), "duration");
}

guint
usd_timeline_get_duration (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), 0);

  priv = USD_TIMELINE_GET_PRIV (timeline);

  return priv->duration;
}

/**
 * usd_timeline_get_direction:
 * @timeline: A #UsdTimeline
 *
 * Returns the direction of the timeline.
 *
 * Return Value: direction
 **/
UsdTimelineDirection
usd_timeline_get_direction (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), USD_TIMELINE_DIRECTION_FORWARD);

  priv = USD_TIMELINE_GET_PRIV (timeline);
  return priv->direction;
}

/**
 * usd_timeline_set_direction:
 * @timeline: A #UsdTimeline
 * @direction: direction
 *
 * Sets the direction of the timeline.
 **/
void
usd_timeline_set_direction (UsdTimeline          *timeline,
			    UsdTimelineDirection  direction)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);
  priv->direction = direction;

  g_object_notify (G_OBJECT (timeline), "direction");
}

GdkScreen *
usd_timeline_get_screen (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), NULL);

  priv = USD_TIMELINE_GET_PRIV (timeline);
  return priv->screen;
}

void
usd_timeline_set_screen (UsdTimeline *timeline,
			 GdkScreen   *screen)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (priv->screen)
    g_object_unref (priv->screen);

  priv->screen = g_object_ref (screen);

  g_object_notify (G_OBJECT (timeline), "screen");
}

void
usd_timeline_set_progress_type (UsdTimeline             *timeline,
				UsdTimelineProgressType  type)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);

  priv->progress_type = type;

  g_object_notify (G_OBJECT (timeline), "progress-type");
}

UsdTimelineProgressType
usd_timeline_get_progress_type (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), USD_TIMELINE_PROGRESS_LINEAR);

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (priv->progress_func)
    return USD_TIMELINE_PROGRESS_LINEAR;

  return priv->progress_type;
}

/**
 * usd_timeline_set_progress_func:
 * @timeline: A #UsdTimeline
 * @progress_func: progress function
 *
 * Sets the progress function. This function will be used to calculate
 * a different progress to pass to the ::frame signal based on the
 * linear progress through the timeline. Setting progress_func
 * to %NULL will make the timeline use the default function,
 * which is just a linear progress.
 *
 * All progresses are in the [0.0, 1.0] range.
 **/
void
usd_timeline_set_progress_func (UsdTimeline             *timeline,
				UsdTimelineProgressFunc  progress_func)
{
  UsdTimelinePriv *priv;

  g_return_if_fail (USD_IS_TIMELINE (timeline));

  priv = USD_TIMELINE_GET_PRIV (timeline);
  priv->progress_func = progress_func;
}

gdouble
usd_timeline_get_progress (UsdTimeline *timeline)
{
  UsdTimelinePriv *priv;
  UsdTimelineProgressFunc progress_func = NULL;
  gdouble linear_progress, progress;
  guint elapsed_time;

  g_return_val_if_fail (USD_IS_TIMELINE (timeline), 0.0);

  priv = USD_TIMELINE_GET_PRIV (timeline);

  if (!priv->timer)
    return 0.;

  elapsed_time = (guint) (g_timer_elapsed (priv->timer, NULL) * 1000);

  linear_progress = (gdouble) elapsed_time / priv->duration;

  if (priv->direction == USD_TIMELINE_DIRECTION_BACKWARD)
    linear_progress = 1 - linear_progress;

  linear_progress = CLAMP (linear_progress, 0., 1.);

  if (priv->progress_func)
    progress_func = priv->progress_func;
  else if (priv->progress_type)
    progress_func = progress_type_to_func (priv->progress_type);

  if (progress_func)
    progress = (progress_func) (linear_progress);
  else
    progress = linear_progress;

  return CLAMP (progress, 0., 1.);
}
