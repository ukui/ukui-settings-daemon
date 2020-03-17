#include "ukui-osd-window.h"

void UkuiOsdWindow::remove_hide_timeout ()
{
    if (hide_timeout_id != 0) {
        g_source_remove (hide_timeout_id);
        hide_timeout_id = 0;
    }

    if (fade_timeout_id != 0) {
        g_source_remove (fade_timeout_id);
        fade_timeout_id = 0;
        fade_out_alpha = 1.0;
    }
}

void UkuiOsdWindow::add_hide_timeout ()
{
    int timeout;

    if (this->is_composited) {
        timeout = DIALOG_FADE_TIMEOUT;
    } else {
        timeout = DIALOG_TIMEOUT;
    }
    hide_timeout_id = g_timeout_add (timeout,
                                     (GSourceFunc) hide_timeout,
                                     window);
}


gboolean UkuiOsdWindow::hide_timeout ()
{
        if (is_composited) {
                hide_timeout_id = 0;
                fade_timeout_id = g_timeout_add (FADE_TIMEOUT,
                                                               (GSourceFunc) fade_timeout,
                                                               this);
        } else {
                gtk_widget_hide (GTK_WIDGET (this->parent));
        }

        return FALSE;
}



/**
 * usd_osd_window_is_composited:
 * @window: a #UsdOsdWindow
 *
 * Return value: whether the window was created on a composited screen.
 */
gboolean UkuiOsdWindow::usd_osd_window_is_composited ()
{
    return is_composited;
}

/**
 * usd_osd_window_is_valid:
 * @window: a #UsdOsdWindow
 *
 * Return value: TRUE if the @window's idea of being composited matches whether
 * its current screen is actually composited.
 */
gboolean UkuiOsdWindow::usd_osd_window_is_valid (UkuiOsdWindow *window)
{
    GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (window));
    return gdk_screen_is_composited (screen) == is_composited;
}
/* This is our draw-event handler when the window is in a compositing manager.
 * We draw everything by hand, using Cairo, so that we can have a nice
 * transparent/rounded look.
 */

static void draw_when_composited (GtkWidget *widget, cairo_t *orig_cr)
{
    UsdOsdWindow    *window;
    cairo_t         *cr;
    cairo_surface_t *surface;
    int              width;
    int              height;
    GtkStyleContext *context;

    window = USD_OSD_WINDOW (widget);

    context = gtk_widget_get_style_context (widget);
    cairo_set_operator (orig_cr, CAIRO_OPERATOR_SOURCE);
    gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

    surface = cairo_surface_create_similar (cairo_get_target (orig_cr),
                                            CAIRO_CONTENT_COLOR_ALPHA,
                                            width,
                                            height);

    if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
        goto done;
    }

    cr = cairo_create (surface);
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
        goto done;
    }

    gtk_render_background (context, cr, 0, 0, width, height);
    gtk_render_frame (context, cr, 0, 0, width, height);

    g_signal_emit (window, signals[DRAW_WHEN_COMPOSITED], 0, cr);

    cairo_destroy (cr);

    /* Make sure we have a transparent background */
    cairo_rectangle (orig_cr, 0, 0, width, height);
    cairo_set_source_rgba (orig_cr, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (orig_cr);

    cairo_set_source_surface (orig_cr, surface, 0, 0);
    cairo_paint_with_alpha (orig_cr, window->priv->fade_out_alpha);

done:
    if (surface != NULL) {
        cairo_surface_destroy (surface);
    }
}

/* This is our draw-event handler when the window is *not* in a compositing manager.
 * We just draw a rectangular frame by hand.  We do this with hardcoded drawing code,
 * instead of GtkFrame, to avoid changing the window's internal widget hierarchy:  in
 * either case (composited or non-composited), callers can assume that this works
 * identically to a GtkWindow without any intermediate widgetry.
 */
static void draw_when_not_composited (GtkWidget *widget, cairo_t *cr)
{
    GtkStyleContext *context;
    int width;
    int height;

    gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
    context = gtk_widget_get_style_context (widget);

    gtk_style_context_set_state (context, GTK_STATE_FLAG_ACTIVE);
    gtk_style_context_add_class(context,"usd-osd-window-solid");
    gtk_render_frame (context,
                      cr,
                      0,
                      0,
                      width,
                      height);
}


static gboolean usd_osd_window_draw (GtkWidget *widget,
                                     cairo_t   *cr)
{
    UsdOsdWindow *window;
    GtkWidget *child;

    window = USD_OSD_WINDOW (widget);

    if (window->priv->is_composited)
        draw_when_composited (widget, cr);
    else
        draw_when_not_composited (widget, cr);

    child = gtk_bin_get_child (GTK_BIN (window));
    if (child)
        gtk_container_propagate_draw (GTK_CONTAINER (window), child, cr);

    return FALSE;
}

static void
usd_osd_window_real_show (GtkWidget *widget)
{
    UsdOsdWindow *window;

    if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->show) {
        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->show (widget);
    }

    window = USD_OSD_WINDOW (widget);
    remove_hide_timeout (window);
    add_hide_timeout (window);
}


static void
usd_osd_window_real_hide (GtkWidget *widget)
{
    UsdOsdWindow *window;

    if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->hide) {
        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->hide (this);
    }

    window = USD_OSD_WINDOW (widget);
    remove_hide_timeout (window);
}

static void
usd_osd_window_real_realize (GtkWidget *widget)
{
    GdkScreen *screen;
    GdkVisual *visual;
    cairo_region_t *region;

    screen = gtk_widget_get_screen (widget);
    visual = gdk_screen_get_rgba_visual (screen);

    if (visual == NULL) {
        visual = gdk_screen_get_system_visual (screen);
    }

    gtk_widget_set_visual (widget, visual);

    if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->realize) {
        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->realize (widget);
    }

    /* make the whole window ignore events */
    region = cairo_region_create ();
    gtk_widget_input_shape_combine_region (widget, region);
    cairo_region_destroy (region);
}


static void
usd_osd_window_style_updated (GtkWidget *widget)
{
    GtkStyleContext *context;
    GtkBorder padding;

    GTK_WIDGET_CLASS (usd_osd_window_parent_class)->style_updated (widget);

    /* We set our border width to 12 (per the UKUI standard), plus the
         * padding of the frame that we draw in our expose/draw handler.  This will
         * make our child be 12 pixels away from the frame.
         */

    context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);
    gtk_container_set_border_width (GTK_CONTAINER (widget), 12 + MAX (padding.left, padding.top));
}


static void
usd_osd_window_get_preferred_width (GtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
    GtkStyleContext *context;
    GtkBorder padding;

    GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_width (widget, minimum, natural);

    /* See the comment in usd_osd_window_style_updated() for why we add the padding here */

    context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);

    *minimum += padding.left;
    *natural += padding.left;
}


static void
usd_osd_window_get_preferred_height (GtkWidget *widget,
                                     gint      *minimum,
                                     gint      *natural)
{
    GtkStyleContext *context;
    GtkBorder padding;

    GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_height (widget, minimum, natural);

    /* See the comment in usd_osd_window_style_updated() for why we add the padding here */

    context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);

    *minimum += padding.top;
    *natural += padding.top;
}

static GObject *
usd_osd_window_constructor (GType                  type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_params)
{
    GObject *object;

    object = G_OBJECT_CLASS (usd_osd_window_parent_class)->constructor (type, n_construct_properties, construct_params);

    g_object_set (object,
                  "type", GTK_WINDOW_POPUP,
                  "type-hint", GDK_WINDOW_TYPE_HINT_NOTIFICATION,
                  "skip-taskbar-hint", TRUE,
                  "skip-pager-hint", TRUE,
                  "focus-on-map", FALSE,
                  NULL);

    GtkWidget *widget = GTK_WIDGET (object);
    GtkStyleContext *style_context = gtk_widget_get_style_context (widget);
    gtk_style_context_add_class (style_context, "osd");

    return object;
}

UkuiOsdWindow::UkuiOsdWindow():is_composited (1)
{
    GdkScreen *screen;
    is_composited = gdk_screen_is_composited (screen);

    if (is_composited) {
        gdouble scalew, scaleh, scale;
        gint size;

        gtk_window_set_decorated (GTK_WINDOW (this), FALSE);
        gtk_widget_set_app_paintable (GTK_WIDGET (this), TRUE);

        GtkStyleContext *style = gtk_widget_get_style_context (GTK_WIDGET (this));
        gtk_style_context_add_class (style, "window-frame");

        /* assume 130x130 on a 640x480 display and scale from there */
        scalew = gdk_screen_get_width (screen) / 640.0;
        scaleh = gdk_screen_get_height (screen) / 480.0;
        scale = MIN (scalew, scaleh);
        size = 130 * MAX (1, scale);

        gtk_window_set_default_size (GTK_WINDOW (this), size, size);

        fade_out_alpha = 1.0;
    } else {
        gtk_container_set_border_width (GTK_CONTAINER (this), 12);
    }

}


/**
 * usd_osd_window_update_and_hide:
 * @window: a #UsdOsdWindow
 *
 * Queues the @window for immediate drawing, and queues a timer to hide the window.
 */
void UkuiOsdWindow::usd_osd_window_update_and_hide ()
{
    remove_hide_timeout (this);
    add_hide_timeout (this);
    if (is_composited) {
        gtk_widget_queue_draw (GTK_WIDGET (this));
    }
}
