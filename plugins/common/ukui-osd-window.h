#ifndef UKUIOSDWINDOW_H
#define UKUIOSDWINDOW_H
#include <glib-object.h>
#include "common_global.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

class COMMONSHARED_EXPORT UkuiOsdWindow
{
public:
    GtkWindow                   parent;

    UkuiOsdWindow();
    guint                    is_composited;
    guint                    hide_timeout_id;
    guint                    fade_timeout_id;
    double                   fade_out_alpha;

gboolean usd_osd_window_is_valid (UkuiOsdWindow *window);
gboolean usd_osd_window_is_composited ();
void usd_osd_window_update_and_hide ();


void add_hide_timeout ();
gboolean hide_timeout ();
void remove_hide_timeout ();
};

#endif // UKUIOSDWINDOW_H
