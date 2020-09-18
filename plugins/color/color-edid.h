#ifndef COLOREDID_H
#define COLOREDID_H

#include <QObject>
#include <colord.h>
extern "C"{
#include <libgnome-desktop/gnome-pnp-ids.h>
}

#define USD_COLOR_TEMPERATURE_MIN               1000    /* Kelvin */
#define USD_COLOR_TEMPERATURE_DEFAULT           6500    /* Kelvin, is RGB [1.0,1.0,1.0] */
#define USD_COLOR_TEMPERATURE_MAX               10000   /* Kelvin */

class ColorEdid
{
public:
    ColorEdid();

    void             EdidReset                  ();
    gboolean         EdidParse                  (const guint8   *data,
                                                 gsize          length);
    const gchar      *EdidGetMonitorName        ();
    const gchar      *EdidGetVendorName         ();
    const gchar      *EdidGetSerialNumber       ();
    const gchar      *EdidGetEisaId             ();
    const gchar      *EdidGetChecksum           ();
    const gchar      *EdidGetPnpId              ();
    guint             EdidGetWidth              ();
    guint             EdidGetHeight             ();
    gfloat            EdidGetGamma              ();
    const CdColorYxy *EdidGetRed                ();
    const CdColorYxy *EdidGetGreen              ();
    const CdColorYxy *EdidGetBlue               ();
    const CdColorYxy *EdidGetWhite              ();

private:
    gchar                   *monitor_name;
    gchar                   *vendor_name;
    gchar                   *serial_number;
    gchar                   *eisa_id;
    gchar                   *checksum;
    gchar                   *pnp_id;
    guint                    width;
    guint                    height;
    gfloat                   gamma;
    CdColorYxy              *red;
    CdColorYxy              *green;
    CdColorYxy              *blue;
    CdColorYxy              *white;
    GnomePnpIds             *pnp_ids;

};

#endif // COLOREDID_H
