#include "ukui-osd-window.h"
#include <glib.h>

void UkuiOsdWindow::usd_osd_window_real_show(UkuiOsdWindow *widget)
{
    UkuiOsdWindow::remove_hide_timeout (widget);
    UkuiOsdWindow::add_hide_timeout (widget);
}

void UkuiOsdWindow::usd_osd_window_real_hide (UkuiOsdWindow *widget)
{
    UkuiOsdWindow::remove_hide_timeout (widget);
}

void UkuiOsdWindow::usd_osd_window_real_realize (UkuiOsdWindow *widget)
{
    GdkScreen *screen;
    GdkVisual *visual;
    cairo_region_t *region;
    GtkWidget *w;

    screen = gtk_widget_get_screen (w);
    visual = gdk_screen_get_rgba_visual (screen);

    if (visual == NULL) {
        visual = gdk_screen_get_system_visual (screen);
    }

    gtk_widget_set_visual (w, visual);

    /* make the whole window ignore events */
    region = cairo_region_create ();
    gtk_widget_input_shape_combine_region (w, region);
    cairo_region_destroy (region);
}

void UkuiOsdWindow::usd_osd_window_style_updated (UkuiOsdWindow *widget)
{
    GtkStyleContext *context;
    GtkBorder padding;
    GtkWidget *w;

    /* We set our border width to 12 (per the UKUI standard), plus the
         * padding of the frame that we draw in our expose/draw handler.  This will
         * make our child be 12 pixels away from the frame.
         */

    context = gtk_widget_get_style_context (w);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);
    gtk_container_set_border_width (GTK_CONTAINER (widget), 12 + MAX (padding.left, padding.top));
}

void UkuiOsdWindow::usd_osd_window_get_preferred_width (UkuiOsdWindow *widget,
                                                               gint      *minimum,
                                                               gint      *natural)
{
    GtkStyleContext *context;
    GtkBorder padding;

    //GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_width (widget, minimum, natural);

    /* See the comment in usd_osd_window_style_updated() for why we add the padding here */

    //context = gtk_widget_get_style_context (widget);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);

    *minimum += padding.left;
    *natural += padding.left;
}

void UkuiOsdWindow::usd_osd_window_get_preferred_height (UkuiOsdWindow *widget,
                                                         gint      *minimum,
                                                         gint      *natural)
{
    GtkStyleContext *context;
    GtkBorder padding;
    GtkWidget *w;
    // GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_height (widget, minimum, natural);

    /* See the comment in usd_osd_window_style_updated() for why we add the padding here */

    context = gtk_widget_get_style_context (w);
    gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL, &padding);

    *minimum += padding.top;
    *natural += padding.top;
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

gboolean UkuiOsdWindow::fade_timeout (UkuiOsdWindow *pWnd)
{
    if (pWnd->fade_out_alpha <= 0.0) {
        gtk_widget_hide (GTK_WIDGET (pWnd));

        /* Reset it for the next time */
        pWnd->fade_out_alpha = 1.0;
        pWnd->fade_timeout_id = 0;

        return FALSE;
    } else {
        GdkRectangle rect;
        GtkWidget *win = GTK_WIDGET (pWnd);
        GtkAllocation allocation;

        pWnd->fade_out_alpha -= 0.10;

        rect.x = 0;
        rect.y = 0;
        gtk_widget_get_allocation (win, &allocation);
        rect.width = allocation.width;
        rect.height = allocation.height;

        gtk_widget_realize (win);
        gdk_window_invalidate_rect (gtk_widget_get_window (win), &rect, FALSE);
    }

    return TRUE;
}

void UkuiOsdWindow::remove_hide_timeout (UkuiOsdWindow *pWnd)
{
    if (pWnd->hide_timeout_id != 0) {
        g_source_remove (pWnd->hide_timeout_id);
        pWnd->hide_timeout_id = 0;
    }

    if (pWnd->fade_timeout_id != 0) {
        g_source_remove (pWnd->fade_timeout_id);
        pWnd->fade_timeout_id = 0;
        pWnd->fade_out_alpha = 1.0;
    }
}

void UkuiOsdWindow::add_hide_timeout (UkuiOsdWindow *pWnd)
{
    int timeout;

    if (pWnd->is_composited) {
        timeout = DIALOG_FADE_TIMEOUT;
    } else {
        timeout = DIALOG_TIMEOUT;
    }
    pWnd->hide_timeout_id = g_timeout_add (timeout,
                                           (GSourceFunc) UkuiOsdWindow::hide_timeout,
                                           pWnd);
}

gboolean UkuiOsdWindow::hide_timeout (UkuiOsdWindow *pWnd)
{
    if (pWnd->is_composited) {
        pWnd->hide_timeout_id = 0;
        pWnd->fade_timeout_id = g_timeout_add (FADE_TIMEOUT,
                                               (GSourceFunc) UkuiOsdWindow::fade_timeout,
                                               pWnd);
    } else {
        gtk_widget_hide (GTK_WIDGET (&(pWnd->parent)));
    }

    return FALSE;
}

///////////////////////////////////////////////

/* This is our draw-event handler when the window is in a compositing manager.
 * We draw everything by hand, using Cairo, so that we can have a nice
 * transparent/rounded look.
 */

static void draw_when_composited (UkuiOsdWindow *widget, cairo_t *orig_cr)
{
    UkuiOsdWindow    *window;
    cairo_t         *cr;
    cairo_surface_t *surface;
    int              width;
    int              height;
    GtkStyleContext *context;

    //window = USD_OSD_WINDOW (widget);

    //context = gtk_widget_get_style_context (widget);
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

    //FIXME
    // gtk_render_background (context, cr, 0, 0, width, height);
     // gtk_render_frame (context, cr, 0, 0, width, height);

   // g_signal_emit (window, signals[DRAW_WHEN_COMPOSITED], 0, cr);

    cairo_destroy (cr);

    /* Make sure we have a transparent background */
    cairo_rectangle (orig_cr, 0, 0, width, height);
    cairo_set_source_rgba (orig_cr, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (orig_cr);

    cairo_set_source_surface (orig_cr, surface, 0, 0);
    cairo_paint_with_alpha (orig_cr, window->fade_out_alpha);

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
static void draw_when_not_composited (UkuiOsdWindow *widget, cairo_t *cr)
{
    GtkStyleContext *context;
    int width;
    int height;

    gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
    /*context = gtk_widget_get_style_context (widget);

    gtk_style_context_set_state (context, GTK_STATE_FLAG_ACTIVE);
    gtk_style_context_add_class(context,"usd-osd-window-solid");
    gtk_render_frame (context,
                      cr,
                      0,
                      0,
                      width,
                      height);
    */
}

static gboolean usd_osd_window_draw (UkuiOsdWindow *widget,
                                     cairo_t   *cr)
{
    UkuiOsdWindow *window;
    GtkWidget *child;

    window = dynamic_cast<UkuiOsdWindow *> (widget);

    if (window->is_composited)
        draw_when_composited (widget, cr);
    else
        draw_when_not_composited (widget, cr);

    child = gtk_bin_get_child (GTK_BIN (window));
    if (child)
        gtk_container_propagate_draw (GTK_CONTAINER (window), child, cr);

    return FALSE;
}

static GObject *
usd_osd_window_constructor (GType                  type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_params)
{
    GObject *object;

    // object = G_OBJECT_CLASS (usd_osd_window_parent_class)->constructor (type, n_construct_properties, construct_params);

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
    parent = this;
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


UkuiOsdWindowClass* UkuiOsdWindowClass::ukui_osd_window_class_init() {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->constructor = usd_osd_window_constructor;

    widget_class->show = usd_osd_window_real_show;
    widget_class->hide = usd_osd_window_real_hide;
    widget_class->realize = usd_osd_window_real_realize;
    widget_class->style_updated = usd_osd_window_style_updated;
    widget_class->get_preferred_width = usd_osd_window_get_preferred_width;
    widget_class->get_preferred_height = usd_osd_window_get_preferred_height;
    widget_class->draw = usd_osd_window_draw;

    signals[DRAW_WHEN_COMPOSITED] = g_signal_new ("draw-when-composited",
                                                    G_TYPE_FROM_CLASS (gobject_class),
                                                    G_SIGNAL_RUN_FIRST,
                                                    G_STRUCT_OFFSET (UsdOsdWindowClass, draw_when_composited),
                                                    NULL, NULL,
                                                    g_cclosure_marshal_VOID__POINTER,
                                                    G_TYPE_NONE, 1,
                                                    G_TYPE_POINTER);

#if GTK_CHECK_VERSION (3, 20, 0)
    gtk_widget_class_set_css_name (widget_class, "UsdOsdWindow");
#endif
    g_type_class_add_private (klass, sizeof (UsdOsdWindowPrivate));

}
