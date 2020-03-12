#ifndef XSETTINGSCOMMON_H
#define XSETTINGSCOMMON_H

#define MOUSE_SCHEMA          "org.ukui.peripherals-mouse"
#define INTERFACE_SCHEMA      "org.mate.interface"
#define SOUND_SCHEMA          "org.mate.sound"

#define CURSOR_THEME_KEY      "cursor-theme"
#define CURSOR_SIZE_KEY       "cursor-size"

#define FONT_RENDER_SCHEMA    "org.ukui.font-rendering"
#define FONT_ANTIALIASING_KEY "antialiasing"
#define FONT_HINTING_KEY      "hinting"
#define FONT_RGBA_ORDER_KEY   "rgba-order"
#define FONT_DPI_KEY          "dpi"

/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_gsettings_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

#endif // XSETTINGSCOMMON_H
