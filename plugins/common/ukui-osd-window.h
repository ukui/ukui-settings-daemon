#ifndef UKUIOSDWINDOW_H
#define UKUIOSDWINDOW_H
#include <glib-object.h>
#include "common_global.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

class UkuiOsdWindowClass;

class UkuiOsdWindow:public GtkWindow
{
public:
    GtkWindow                   *parent;
    UkuiOsdWindowClass          *pClass;
    UkuiOsdWindow();
    guint                    is_composited;
    guint                    hide_timeout_id;
    guint                    fade_timeout_id;
    double                   fade_out_alpha;


    static void usd_osd_window_real_show (UkuiOsdWindow *widget);
    static void usd_osd_window_real_hide (UkuiOsdWindow *widget);
    static void usd_osd_window_real_realize (UkuiOsdWindow *widget);
    static void usd_osd_window_style_updated (UkuiOsdWindow *widget);
    static void usd_osd_window_get_preferred_width (UkuiOsdWindow *widget,
                                                    gint      *minimum,
                                                    gint      *natural);
    static void usd_osd_window_get_preferred_height (UkuiOsdWindow *widget,
                                                     gint      *minimum,
                                                     gint      *natural);

    gboolean usd_osd_window_is_valid (UkuiOsdWindow *window);
    gboolean usd_osd_window_is_composited ();
    void usd_osd_window_update_and_hide ();

    static gboolean fade_timeout (UkuiOsdWindow *pWnd);
    static void add_hide_timeout (UkuiOsdWindow *pWnd);
    static gboolean hide_timeout (UkuiOsdWindow *pWnd);
    static void remove_hide_timeout (UkuiOsdWindow *pWnd);

};

class UkuiOsdWindowClass :public GtkWindowClass{
    static UkuiOsdWindowClass* ukui_osd_window_class_init(UkuiOsdWindowClass*);
    void (* draw_when_composited) (UkuiOsdWindow *window, cairo_t *cr);
};

#endif // UKUIOSDWINDOW_H
