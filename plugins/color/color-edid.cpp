#include <QDebug>
#include "color-edid.h"
#include <math.h>
#include <string.h>
#include <gio/gio.h>
#include <stdlib.h>

#define EDID_OFFSET_PNPID                           0x08
#define EDID_OFFSET_SERIAL                          0x0c
#define EDID_OFFSET_SIZE                            0x15
#define EDID_OFFSET_GAMMA                           0x17
#define EDID_OFFSET_DATA_BLOCKS                     0x36
#define EDID_OFFSET_LAST_BLOCK                      0x6c
#define EDID_OFFSET_EXTENSION_BLOCK_COUNT           0x7e

#define DESCRIPTOR_DISPLAY_PRODUCT_NAME             0xfc
#define DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER    0xff
#define DESCRIPTOR_COLOR_MANAGEMENT_DATA            0xf9
#define DESCRIPTOR_ALPHANUMERIC_DATA_STRING         0xfe
#define DESCRIPTOR_COLOR_POINT                      0xfb

ColorEdid::ColorEdid()
{
    pnp_ids = gnome_pnp_ids_new ();
    pnp_id = g_new0 (gchar, 4);
    red = cd_color_yxy_new ();
    green = cd_color_yxy_new ();
    blue = cd_color_yxy_new ();
    white = cd_color_yxy_new ();
}


const gchar *ColorEdid::EdidGetMonitorName ()
{
    return monitor_name;
}

const gchar *ColorEdid::EdidGetVendorName ()
{
    if (vendor_name == NULL)
            vendor_name = gnome_pnp_ids_get_pnp_id (pnp_ids, pnp_id);
    return vendor_name;
}

const gchar *ColorEdid::EdidGetSerialNumber ()
{
    return serial_number;
}

const gchar *ColorEdid::EdidGetEisaId ()
{
    return eisa_id;
}

const gchar *ColorEdid::EdidGetChecksum ()
{
    return checksum;
}

const gchar *ColorEdid::EdidGetPnpId ()
{
    return pnp_id;
}

guint ColorEdid::EdidGetWidth ()
{
    return width;
}

guint ColorEdid::EdidGetHeight ()
{
    return height;
}

gfloat ColorEdid::EdidGetGamma ()
{
    return gamma;
}

const CdColorYxy *ColorEdid::EdidGetRed ()
{
    return red;
}

const CdColorYxy *ColorEdid::EdidGetGreen ()
{
    return green;
}

const CdColorYxy *ColorEdid::EdidGetBlue ()
{
    return blue;
}

const CdColorYxy *ColorEdid::EdidGetWhite ()
{
    return white;
}

void ColorEdid::EdidReset ()
{
    /* free old data */
    /*
    g_free (monitor_name);
    g_free (vendor_name);
    g_free (serial_number);
    g_free (eisa_id);
    g_free (checksum);
    */
    /* do not deallocate, just blank */
    pnp_id[0] = '\0';
    /* set to default values */
    monitor_name = NULL;
    vendor_name = NULL;
    serial_number = NULL;
    eisa_id = NULL;
    checksum = NULL;
    width = 0;
    height = 0;
    gamma = 0.0f;
}

static gint
EdidGetBit (gint in, gint bit)
{
        return (in & (1 << bit)) >> bit;
}

/**
 * EdidGetBits:
 **/
static gint
EdidGetBits (gint in, gint begin, gint end)
{
    gint mask = (1 << (end - begin + 1)) - 1;

    return (in >> begin) & mask;
}

/**
 * edid_decode_fraction:
 **/
static gdouble
EdidDecodeFraction (gint high, gint low)
{
        gdouble result = 0.0;
        gint i;

        high = (high << 2) | low;
        for (i = 0; i < 10; ++i)
                result += EdidGetBit (high, i) * pow (2, i - 10);
        return result;
}

static gchar *
EdidParseString (const guint8 *data)
{
        gchar *text;
        guint i;
        guint replaced = 0;

        /* this is always 13 bytes, but we can't guarantee it's null
         * terminated or not junk. */
        text = g_strndup ((const gchar *) data, 13);

        /* remove insane newline chars */
        g_strdelimit (text, "\n\r", '\0');

        /* remove spaces */
        g_strchomp (text);

        /* nothing left? */
        if (text[0] == '\0') {
                g_free (text);
                text = NULL;
                goto out;
        }

        /* ensure string is printable */
        for (i = 0; text[i] != '\0'; i++) {
                if (!g_ascii_isprint (text[i])) {
                        text[i] = '-';
                        replaced++;
                }
        }

        /* if the string is junk, ignore the string */
        if (replaced > 4) {
                g_free (text);
                text = NULL;
                goto out;
        }
out:
        return text;
}

gboolean ColorEdid::EdidParse (const guint8 *data, gsize length)
{
    gboolean ret = TRUE;
    guint i;
    guint32 serial;
    gchar *tmp;

    /* check header */
    if (length < 128) {
            qDebug("EDID length is too small");
            ret = FALSE;
            goto out;
    }
    if (data[0] != 0x00 || data[1] != 0xff) {
            qDebug("Failed to parse EDID header");
            ret = FALSE;
            goto out;
    }

    /* free old data */
    EdidReset ();

    /* decode the PNP ID from three 5 bit words packed into 2 bytes
     * /--08--\/--09--\
     * 7654321076543210
     * |\---/\---/\---/
     * R  C1   C2   C3 */
    pnp_id[0] = 'A' + ((data[EDID_OFFSET_PNPID+0] & 0x7c) / 4) - 1;
    pnp_id[1] = 'A' + ((data[EDID_OFFSET_PNPID+0] & 0x3) * 8) + ((data[EDID_OFFSET_PNPID+1] & 0xe0) / 32) - 1;
    pnp_id[2] = 'A' + (data[EDID_OFFSET_PNPID+1] & 0x1f) - 1;

    /* maybe there isn't a ASCII serial number descriptor, so use this instead */
    serial = (guint32) data[EDID_OFFSET_SERIAL+0];
    serial += (guint32) data[EDID_OFFSET_SERIAL+1] * 0x100;
    serial += (guint32) data[EDID_OFFSET_SERIAL+2] * 0x10000;
    serial += (guint32) data[EDID_OFFSET_SERIAL+3] * 0x1000000;
    if (serial > 0)
            serial_number = g_strdup_printf ("%" G_GUINT32_FORMAT, serial);

    /* get the size */
    width = data[EDID_OFFSET_SIZE+0];
    height = data[EDID_OFFSET_SIZE+1];

    /* we don't care about aspect */
    if (width == 0 || height == 0) {
            width = 0;
            height = 0;
    }

    /* get gamma */
    if (data[EDID_OFFSET_GAMMA] == 0xff) {
            gamma = 1.0f;
    } else {
            gamma = ((gfloat) data[EDID_OFFSET_GAMMA] / 100) + 1;
    }

    /* get color red */
    red->x = EdidDecodeFraction (data[0x1b], EdidGetBits (data[0x19], 6, 7));
    red->y = EdidDecodeFraction (data[0x1c], EdidGetBits (data[0x19], 4, 5));

    /* get color green */
    green->x = EdidDecodeFraction (data[0x1d], EdidGetBits (data[0x19], 2, 3));
    green->y = EdidDecodeFraction (data[0x1e], EdidGetBits (data[0x19], 0, 1));

    /* get color blue */
    blue->x = EdidDecodeFraction (data[0x1f], EdidGetBits (data[0x1a], 6, 7));
    blue->y = EdidDecodeFraction (data[0x20], EdidGetBits (data[0x1a], 4, 5));

    /* get color white */
    white->x = EdidDecodeFraction (data[0x21], EdidGetBits (data[0x1a], 2, 3));
    white->y = EdidDecodeFraction (data[0x22], EdidGetBits (data[0x1a], 0, 1));

    /* parse EDID data */
    for (i = EDID_OFFSET_DATA_BLOCKS;
         i <= EDID_OFFSET_LAST_BLOCK;
         i += 18) {
            /* ignore pixel clock data */
            if (data[i] != 0)
                    continue;
            if (data[i+2] != 0)
                    continue;

            /* any useful blocks? */
            if (data[i+3] == DESCRIPTOR_DISPLAY_PRODUCT_NAME) {
                    tmp = EdidParseString (&data[i+5]);
                    if (tmp != NULL) {
                            g_free (monitor_name);
                            monitor_name = tmp;
                    }
            } else if (data[i+3] == DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER) {
                    tmp = EdidParseString (&data[i+5]);
                    if (tmp != NULL) {
                            g_free (serial_number);
                            serial_number = tmp;
                    }
            } else if (data[i+3] == DESCRIPTOR_COLOR_MANAGEMENT_DATA) {
                    g_warning ("failing to parse color management data");
            } else if (data[i+3] == DESCRIPTOR_ALPHANUMERIC_DATA_STRING) {
                    tmp = EdidParseString (&data[i+5]);
                    if (tmp != NULL) {
                            g_free (eisa_id);
                            eisa_id = tmp;
                    }
            } else if (data[i+3] == DESCRIPTOR_COLOR_POINT) {
                    if (data[i+3+9] != 0xff) {
                            /* extended EDID block(1) which contains
                             * a better gamma value */
                            gamma = ((gfloat) data[i+3+9] / 100) + 1;
                    }
                    if (data[i+3+14] != 0xff) {
                            /* extended EDID block(2) which contains
                             * a better gamma value */
                            gamma = ((gfloat) data[i+3+9] / 100) + 1;
                    }
            }
    }

    /* calculate checksum */
    checksum = g_compute_checksum_for_data (G_CHECKSUM_MD5, data, length);
out:
    return ret;
}
