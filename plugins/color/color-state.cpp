#include "color-state.h"
#include "config.h"

typedef struct {
        guint32          red;
        guint32          green;
        guint32          blue;
} MateRROutputClutItem;

typedef struct {
        ColorState          *state;
        CdProfile           *profile;
        CdDevice            *device;
        guint32             output_id;
} SessionAsyncHelper;

/* see http://www.oyranos.org/wiki/index.php?title=ICC_Profiles_in_X_Specification_0.3 */
#define USD_ICC_PROFILE_IN_X_VERSION_MAJOR      0
#define USD_ICC_PROFILE_IN_X_VERSION_MINOR      3

ColorState::ColorState()
{
#ifdef GDK_WINDOWING_X11
   /* set the _ICC_PROFILE atoms on the root screen */
   if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
           gdk_window = gdk_screen_get_root_window (gdk_screen_get_default ());
#endif

   /* parsing the EDID is expensive */
   edid_cache = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_object_unref);

   /* we don't want to assign devices multiple times at startup */
   device_assign_hash = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               NULL);

   /* default color temperature */
   color_temperature = USD_COLOR_TEMPERATURE_DEFAULT;

   client = cd_client_new ();
   cancellable = nullptr;
}

ColorState::~ColorState()
{
    g_cancellable_cancel (cancellable);
    g_clear_object (&cancellable);
    g_clear_object (&client);
    g_clear_pointer (&edid_cache, g_hash_table_destroy);
    g_clear_pointer (&device_assign_hash, g_hash_table_destroy);
    g_clear_object (&state_screen);
}
void ColorState::ColorStateSetTemperature(guint temperature)
{
    if (color_temperature == temperature)
                    return;
    if(temperature < USD_COLOR_TEMPERATURE_MIN)
        temperature = USD_COLOR_TEMPERATURE_MIN;
    if(temperature > USD_COLOR_TEMPERATURE_MAX)
        temperature = USD_COLOR_TEMPERATURE_MAX;
    color_temperature = temperature;
    SessionSetGammaForAllDevices (this);
}

ColorEdid *ColorState::SessionGetOutputEdid (ColorState *state,
                                             MateRROutput *output)
{
        const guint8 *data;
        gsize size = 128;
        ColorEdid *edid = NULL;
        gboolean ret;

        /* can we find it in the cache */
        edid = (ColorEdid *)g_hash_table_lookup (state->edid_cache,
                                    mate_rr_output_get_name (output));
        if (edid != NULL) {
                return edid;
        }

        /* parse edid */
        data = mate_rr_output_get_edid_data (output);
        if (data == nullptr) {
               qDebug("unable to get EDID for output");
                return nullptr;
        }
        edid = new ColorEdid ();
        ret = edid->EdidParse (data, size);
        if (!ret) {
                delete edid;
                return nullptr;
        }

        /* add to cache */
        g_hash_table_insert (state->edid_cache,
                             g_strdup (mate_rr_output_get_name (output)),
                             edid);

        return edid;
}

gchar *ColorState::SessionGetOutputId (ColorState *state, MateRROutput *output)
{
        const gchar *name;
        const gchar *serial;
        const gchar *vendor;
        ColorEdid *edid = NULL;
        GString *device_id;

        /* all output devices are prefixed with this */
        device_id = g_string_new ("xrandr");

        /* get the output EDID if possible */
        edid = SessionGetOutputEdid (state, output);
        if (edid == NULL) {
                qDebug ("no edid for %s, falling back to connection name",
                         mate_rr_output_get_name (output));
                g_string_append_printf (device_id,
                                        "-%s",
                                        mate_rr_output_get_name (output));
                goto out;
        }

        /* check EDID data is okay to use */
        vendor = edid->EdidGetVendorName();
        name = edid->EdidGetMonitorName();
        serial = edid->EdidGetSerialNumber();

        if (vendor == NULL && name == NULL && serial == NULL) {
            qDebug ("edid invalid for %s, falling back to connection name",
                     mate_rr_output_get_name (output));
            g_string_append_printf (device_id,
                                    "-%s",
                                    mate_rr_output_get_name (output));
            goto out;
        }

        /* use EDID data */
        if (vendor != NULL)
                g_string_append_printf (device_id, "-%s", vendor);
        if (name != NULL)
                g_string_append_printf (device_id, "-%s", name);
        if (serial != NULL)
                g_string_append_printf (device_id, "-%s", serial);
out:
        if (edid != NULL)
            edid = nullptr;
        return g_string_free (device_id, FALSE);
}


static void
SessionCreateDeviceCb (GObject *object,
                              GAsyncResult *res,
                              gpointer user_data)
{
        CdDevice *device;
        GError *error = NULL;

        device = cd_client_create_device_finish (CD_CLIENT (object),
                                                 res,
                                                 &error);
        if (device == NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
                    !g_error_matches (error, CD_CLIENT_ERROR, CD_CLIENT_ERROR_ALREADY_EXISTS))
                {
                        qWarning ("failed to create device: %s", error->message);
                }
                g_error_free (error);
                return;
        }
        fprintf(stderr, "success to create device\n");
        g_object_unref (device);
}

void ColorState::SessionAddStateOutput (ColorState *state, MateRROutput *output)
{
        const gchar *edid_checksum = NULL;
        const gchar *model = NULL;
        const gchar *output_name = NULL;
        const gchar *serial = NULL;
        const gchar *vendor = NULL;
        gboolean ret;
        gchar *device_id = NULL;
        ColorEdid *edid;
        GHashTable *device_props = NULL;

        /* VNC creates a fake device that cannot be color managed */
        output_name = mate_rr_output_get_name (output);
        if (output_name != NULL && g_str_has_prefix (output_name, "VNC-")) {
                qDebug ("ignoring %s as fake VNC device detected", output_name);
                return;
        }

        /* try to get edid */
        edid = SessionGetOutputEdid (state, output);
        if (edid == nullptr) {
                qWarning ("failed to get edid:");
        }

        /* prefer DMI data for the internal output */
        ret = mate_rr_output_is_laptop (output);
        if (ret) {
                model = cd_client_get_system_model (state->client);
                vendor = cd_client_get_system_vendor (state->client);
        }

        /* use EDID data if we have it */
        if (edid != nullptr) {
                edid_checksum = edid->EdidGetChecksum();
                if (model == NULL)
                        model = edid->EdidGetMonitorName();
                if (vendor == NULL)
                        vendor = edid->EdidGetVendorName();
                if (serial == NULL)
                        serial = edid->EdidGetSerialNumber();
        }

        /* ensure mandatory fields are set */
        if (model == NULL)
                model = mate_rr_output_get_name (output);
        if (vendor == NULL)
                vendor = "unknown";
        if (serial == NULL)
                serial = "unknown";

        device_id = SessionGetOutputId (state, output);
        qDebug ("output %s added", device_id);
        device_props = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              NULL, NULL);
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_KIND,
                             (gpointer) cd_device_kind_to_string (CD_DEVICE_KIND_DISPLAY));
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_MODE,
                             (gpointer) cd_device_mode_to_string (CD_DEVICE_MODE_PHYSICAL));
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_COLORSPACE,
                             (gpointer) cd_colorspace_to_string (CD_COLORSPACE_RGB));
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_VENDOR,
                             (gpointer) vendor);
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_MODEL,
                             (gpointer) model);
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_PROPERTY_SERIAL,
                             (gpointer) serial);
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_METADATA_XRANDR_NAME,
                             (gpointer) mate_rr_output_get_name (output));
        g_hash_table_insert (device_props,
                             (gpointer) CD_DEVICE_METADATA_OUTPUT_PRIORITY,
                             mate_rr_output_get_is_primary (output) ?
                             (gpointer) CD_DEVICE_METADATA_OUTPUT_PRIORITY_PRIMARY :
                             (gpointer) CD_DEVICE_METADATA_OUTPUT_PRIORITY_SECONDARY);
        if (edid_checksum != NULL) {
                g_hash_table_insert (device_props,
                                     (gpointer) CD_DEVICE_METADATA_OUTPUT_EDID_MD5,
                                     (gpointer) edid_checksum);
        }
        /* set this so we can call the device a 'Laptop Screen' in the
         * control center main panel */
        if (mate_rr_output_is_laptop (output)) {
                g_hash_table_insert (device_props,
                                     (gpointer) CD_DEVICE_PROPERTY_EMBEDDED,
                                     NULL);
        }
        cd_client_create_device (state->client,
                                 device_id,
                                 CD_OBJECT_SCOPE_TEMP,
                                 device_props,
                                 state->cancellable,
                                 SessionCreateDeviceCb,
                                 state);
        g_free (device_id);
        if (device_props != NULL)
                g_hash_table_unref (device_props);
        if (edid != nullptr)
            edid = nullptr;
}

static guint
MateRrOutputGetGammaSize (MateRROutput *output)
{
        MateRRCrtc *crtc;
        gint len = 0;

        crtc = mate_rr_output_get_crtc (output);
        if (crtc == NULL)
                return 0;
        mate_rr_crtc_get_gamma (crtc,
                                 &len,
                                 NULL, NULL, NULL);
        return (guint) len;
}

static void
SessionAsyncHelperFree (SessionAsyncHelper *helper)
{
        if (helper->profile != NULL)
                g_object_unref (helper->profile);
        if (helper->device != NULL)
                g_object_unref (helper->device);
        g_free (helper);
}

static gboolean
SessionOutputSetGamma (MateRROutput *output,
                       GPtrArray *array)
{
        gboolean ret = TRUE;
        guint16 *red = NULL;
        guint16 *green = NULL;
        guint16 *blue = NULL;
        guint i;
        MateRROutputClutItem *data;
        MateRRCrtc *crtc;

        /* no length? */
        if (array->len == 0) {
            ret = FALSE;
            qDebug("no data in the CLUT array");
            goto out;
        }

        /* convert to a type X understands */
        red = g_new (guint16, array->len);
        green = g_new (guint16, array->len);
        blue = g_new (guint16, array->len);
        for (i = 0; i < array->len; i++) {
                data = (MateRROutputClutItem *)g_ptr_array_index (array, i);
                red[i] = data->red;
                green[i] = data->green;
                blue[i] = data->blue;
        }

        /* send to LUT */
        crtc = mate_rr_output_get_crtc (output);
        if (crtc == NULL) {
            ret = FALSE;
            qDebug("failed to get ctrc for %s",mate_rr_output_get_name (output));
            goto out;
        }
        mate_rr_crtc_set_gamma (crtc, array->len,
                                 red, green, blue);
out:
        g_free (red);
        g_free (green);
        g_free (blue);
        return ret;
}


bool ColorState::SessionDeviceResetGamma(MateRROutput *output,
                                         guint color_temperature)
{
        bool ret;
        guint i;
        guint size;
        guint32 value;
        GPtrArray *clut;
        MateRROutputClutItem *data;
        CdColorRGB temp;

        /* create a linear ramp */
        qDebug ("falling back to dummy ramp");
        clut = g_ptr_array_new_with_free_func (g_free);
        size = MateRrOutputGetGammaSize (output);
        if (size == 0) {
                ret = true;
                goto out;
        }

        /* get the color temperature */
        if (!cd_color_get_blackbody_rgb_full (color_temperature,
                                              &temp,
                                              CD_COLOR_BLACKBODY_FLAG_USE_PLANCKIAN)) {
                qWarning ("failed to get blackbody for %uK", color_temperature);
                cd_color_rgb_set (&temp, 1.0, 1.0, 1.0);
        } else {
                qDebug ("using reset gamma of %uK = %.1f,%.1f,%.1f",
                         color_temperature, temp.R, temp.G, temp.B);
        }

        for (i = 0; i < size; i++) {
                value = (i * 0xffff) / (size - 1);
                data = g_new0 (MateRROutputClutItem, 1);
                data->red = value * temp.R;
                data->green = value * temp.G;
                data->blue = value * temp.B;
                g_ptr_array_add (clut, data);
        }

        /* apply the vcgt to this output */
        ret = SessionOutputSetGamma (output, clut);
        if (!ret)
                goto out;
out:
        g_ptr_array_unref (clut);
        return ret;
}

/*
 * Check to see if the on-disk profile has the MAPPING_device_id
 * metadata, and if not, we should delete the profile and re-create it
 * so that it gets mapped by the daemon.
 */
gboolean ColorState::SessionCheckProfileDeviceMd (GFile *file)
{
        const gchar *key_we_need = CD_PROFILE_METADATA_MAPPING_DEVICE_ID;
        CdIcc *icc;
        gboolean ret;

        icc = cd_icc_new ();
        ret = cd_icc_load_file (icc, file, CD_ICC_LOAD_FLAGS_METADATA, NULL, NULL);
        if (!ret)
                goto out;
        ret = cd_icc_get_metadata_item (icc, key_we_need) != NULL;
        if (!ret) {
                qDebug ("auto-edid profile is old, and contains no %s data",
                         key_we_need);
        }
out:
        g_object_unref (icc);
        return ret;
}


MateRROutput *ColorState::SessionGetStateOutputById (ColorState *state,
                                                     const gchar *device_id)
{
        gchar *output_id;
        MateRROutput *output = NULL;
        MateRROutput **outputs = NULL;
        guint i;

        /* search all STATE outputs for the device id */
        outputs = mate_rr_screen_list_outputs (state->state_screen);
        if (outputs == NULL) {
                qDebug("Failed to get outputs");
                goto out;
        }
        for (i = 0; outputs[i] != NULL && output == NULL; i++) {
                output_id = SessionGetOutputId (state, outputs[i]);
                if (g_strcmp0 (output_id, device_id) == 0)
                        output = outputs[i];
                g_free (output_id);
        }
        if (output == NULL) {
                qDebug("Failed to find output %s",
                             device_id);
        }
out:
        return output;
}


bool ColorState::GetSystemIccProfile(ColorState *state,
                                     GFile *file)
{
        const char efi_path[] = "/sys/firmware/efi/efivars/INTERNAL_PANEL_COLOR_INFO-01e1ada1-79f2-46b3-8d3e-71fc0996ca6b";
        /* efi variables have a 4-byte header */
        const int efi_var_header_length = 4;
        g_autoptr(GFile) efi_file = g_file_new_for_path (efi_path);
        gboolean ret;
        g_autofree char *data = NULL;
        gsize length;
        g_autoptr(GError) error = NULL;

        ret = g_file_query_exists (efi_file, NULL);
        if (!ret)
                return false;

        ret = g_file_load_contents (efi_file,
                                    NULL /* cancellable */,
                                    &data,
                                    &length,
                                    NULL /* etag_out */,
                                    &error);

        if (!ret) {
                qWarning ("failed to read EFI system color profile: %s",
                           error->message);
                return false;
        }

        if (length <= efi_var_header_length) {
                qWarning ("EFI system color profile was too short");
                return false;
        }

        ret = g_file_replace_contents (file,
                                       data + efi_var_header_length,
                                       length - efi_var_header_length,
                                       NULL /* etag */,
                                       FALSE /* make_backup */,
                                       G_FILE_CREATE_NONE,
                                       NULL /* new_etag */,
                                       NULL /* cancellable */,
                                       &error);
        if (!ret) {
                qWarning ("failed to write system color profile: %s",
                           error->message);
                return false;
        }

        return true;
}


static gboolean UtilsMkdirForFilename (GFile *file)
{
        gboolean ret = FALSE;
        GFile *parent_dir = NULL;

        /* get parent directory */
        parent_dir = g_file_get_parent (file);
        if (parent_dir == NULL) {
                qDebug("could not get parent dir");
                goto out;
        }

        /* ensure desination does not already exist */
        ret = g_file_query_exists (parent_dir, NULL);
        if (ret)
                goto out;
        ret = g_file_make_directory_with_parents (parent_dir, NULL, NULL);
        if (!ret)
                goto out;
out:
        if (parent_dir != NULL)
                g_object_unref (parent_dir);
        return ret;
}

bool ColorState::ApplyCreateIccProfileForEdid (ColorState *state,
                                               CdDevice *device,
                                               ColorEdid *edid,
                                               GFile *file,
                                               GError **error)
{
        CdIcc *icc = NULL;
        const gchar *data;
        bool ret = false;

        /* ensure the per-user directory exists */
        ret = UtilsMkdirForFilename (file);
        if (!ret)
                goto out;

        /* create our generated profile */
        icc = cd_icc_new ();
        ret = cd_icc_create_from_edid (icc,
                                       edid->EdidGetGamma(),
                                       edid->EdidGetRed(),
                                       edid->EdidGetGreen(),
                                       edid->EdidGetBlue(),
                                       edid->EdidGetWhite(),
                                       NULL);
        if (!ret)
                goto out;

        /* set copyright */
        cd_icc_set_copyright (icc, NULL,
                              /* deliberately not translated */
                              "This profile is free of known copyright restrictions.");

        /* set model and title */
        data = edid->EdidGetMonitorName();
        if (data == NULL)
                data = cd_client_get_system_model (state->client);
        if (data == NULL)
                data = "Unknown monitor";
        cd_icc_set_model (icc, NULL, data);
        cd_icc_set_description (icc, NULL, data);

        /* get manufacturer */
        data = edid->EdidGetVendorName();
        if (data == NULL)
                data = cd_client_get_system_vendor (state->client);
        if (data == NULL)
                data = "Unknown vendor";
        cd_icc_set_manufacturer (icc, NULL, data);

        /* set the framework creator metadata */
        cd_icc_add_metadata (icc,
                             CD_PROFILE_METADATA_CMF_PRODUCT,
                             PACKAGE_NAME);
        cd_icc_add_metadata (icc,
                             CD_PROFILE_METADATA_CMF_BINARY,
                             PACKAGE_NAME);
        cd_icc_add_metadata (icc,
                             CD_PROFILE_METADATA_CMF_VERSION,
                             PACKAGE_VERSION);
        cd_icc_add_metadata (icc,
                             CD_PROFILE_METADATA_MAPPING_DEVICE_ID,
                             cd_device_get_id (device));

        /* set 'ICC meta Tag for Monitor Profiles' data */
        cd_icc_add_metadata (icc, CD_PROFILE_METADATA_EDID_MD5, edid->EdidGetChecksum());
        data = edid->EdidGetMonitorName();
        if (data != NULL)
                cd_icc_add_metadata (icc, CD_PROFILE_METADATA_EDID_MODEL, data);
        data = edid->EdidGetSerialNumber();
        if (data != NULL)
                cd_icc_add_metadata (icc, CD_PROFILE_METADATA_EDID_SERIAL, data);
        data = edid->EdidGetPnpId();
        if (data != NULL)
                cd_icc_add_metadata (icc, CD_PROFILE_METADATA_EDID_MNFT, data);
        data = edid->EdidGetVendorName();
        if (data != NULL)
                cd_icc_add_metadata (icc, CD_PROFILE_METADATA_EDID_VENDOR, data);

        /* save */
        ret = cd_icc_save_file (icc, file, CD_ICC_SAVE_FLAGS_NONE, NULL, error);
        if (!ret)
                goto out;
out:
        if (icc != NULL)
                g_object_unref (icc);
        return ret;
}

/* this function is more complicated than it should be, due to the
 * fact that XOrg sometimes assigns no primary devices when using
 * "xrandr --auto" or when the version of RANDR is < 1.3 */
bool ColorState::SessionUseOutputProfileForScreen (ColorState *state,
                                                   MateRROutput *output)
{
        gboolean has_laptop = FALSE;
        gboolean has_primary = FALSE;
        MateRROutput **outputs;
        MateRROutput *connected = NULL;
        guint i;

        /* do we have any screens marked as primary */
        outputs = mate_rr_screen_list_outputs (state->state_screen);
        if (outputs == NULL || outputs[0] == NULL) {
                qWarning ("failed to get outputs");
                return false;
        }
        for (i = 0; outputs[i] != NULL; i++) {
                if (connected == NULL)
                        connected = outputs[i];
                if (mate_rr_output_get_is_primary (outputs[i]))
                        has_primary = TRUE;
                if (mate_rr_output_is_laptop (outputs[i]))
                        has_laptop = TRUE;
        }

        /* we have an assigned primary device, are we that? */
        if (has_primary)
                return mate_rr_output_get_is_primary (output);

        /* choosing the internal panel is probably sane */
        if (has_laptop)
                return mate_rr_output_is_laptop (output);

        /* we have to choose one, so go for the first connected device */
        if (connected != NULL)
                return mate_rr_output_get_id (connected) == mate_rr_output_get_id (output);

        return false;
}


bool ColorState::SessionScreenSetIccProfile (ColorState *state,
                                             const gchar *filename,
                                             GError **error)
{
        gchar *data = NULL;
        gsize length;
        guint version_data;

        g_return_val_if_fail (filename != NULL, FALSE);

        /* wayland */
        if (state->gdk_window == NULL) {
                qDebug ("not setting atom as running under wayland");
                return true;
        }

        qDebug ("setting root window ICC profile atom from %s", filename);

        /* get contents of file */
        if (!g_file_get_contents (filename, &data, &length, error))
                return false;

        /* set profile property */
        gdk_property_change (state->gdk_window,
                             gdk_atom_intern_static_string ("_ICC_PROFILE"),
                             gdk_atom_intern_static_string ("CARDINAL"),
                             8,
                             GDK_PROP_MODE_REPLACE,
                             (const guchar *) data, length);

        /* set version property */
        version_data = USD_ICC_PROFILE_IN_X_VERSION_MAJOR * 100 +
                        USD_ICC_PROFILE_IN_X_VERSION_MINOR * 1;
        gdk_property_change (state->gdk_window,
                             gdk_atom_intern_static_string ("_ICC_PROFILE_IN_X_VERSION"),
                             gdk_atom_intern_static_string ("CARDINAL"),
                             8,
                             GDK_PROP_MODE_REPLACE,
                             (const guchar *) &version_data, 1);

        g_free (data);
        return true;
}


static GPtrArray *
SessionGenerateVcgt (CdProfile *profile, guint color_temperature, guint size)
{
        MateRROutputClutItem *tmp;
        GPtrArray *array = NULL;
        const cmsToneCurve **vcgt;
        cmsFloat32Number in;
        guint i;
        cmsHPROFILE lcms_profile;
        CdIcc *icc = NULL;
        CdColorRGB temp;

        /* invalid size */
        if (size == 0)
                goto out;

        /* open file */
        icc = cd_profile_load_icc (profile, CD_ICC_LOAD_FLAGS_NONE, NULL, NULL);
        if (icc == NULL)
                goto out;

        /* get tone curves from profile */
        lcms_profile = cd_icc_get_handle (icc);
        vcgt = (const cmsToneCurve **)cmsReadTag (lcms_profile, cmsSigVcgtTag);
        if (vcgt == NULL || vcgt[0] == NULL) {
                qDebug ("profile does not have any VCGT data");
                goto out;
        }

        /* get the color temperature */
        if (!cd_color_get_blackbody_rgb_full (color_temperature,
                                              &temp,
                                              CD_COLOR_BLACKBODY_FLAG_USE_PLANCKIAN)) {
                qWarning ("failed to get blackbody for %uK", color_temperature);
                cd_color_rgb_set (&temp, 1.0, 1.0, 1.0);
        } else {
                qDebug ("using VCGT gamma of %uK = %.1f,%.1f,%.1f",
                         color_temperature, temp.R, temp.G, temp.B);
        }

        /* create array */
        array = g_ptr_array_new_with_free_func (g_free);
        for (i = 0; i < size; i++) {
                in = (gdouble) i / (gdouble) (size - 1);
                tmp = g_new0 (MateRROutputClutItem, 1);
                tmp->red = cmsEvalToneCurveFloat(vcgt[0], in) * temp.R * (gdouble) 0xffff;
                tmp->green = cmsEvalToneCurveFloat(vcgt[1], in) * temp.G * (gdouble) 0xffff;
                tmp->blue = cmsEvalToneCurveFloat(vcgt[2], in) * temp.B * (gdouble) 0xffff;
                g_ptr_array_add (array, tmp);
        }
out:
        if (icc != NULL)
                g_object_unref (icc);
        return array;
}


static gboolean
SessionDeviceSetGamma (MateRROutput *output,
                       CdProfile *profile,
                       guint color_temperature)
{
        gboolean ret = FALSE;
        guint size;
        GPtrArray *clut = NULL;

        /* create a lookup table */
        size = MateRrOutputGetGammaSize (output);
        if (size == 0) {
                ret = TRUE;
                goto out;
        }
        clut = SessionGenerateVcgt (profile, color_temperature, size);
        if (clut == NULL) {
                qDebug("failed to generate vcgt");
                goto out;
        }

        /* apply the vcgt to this output */
        ret = SessionOutputSetGamma (output, clut);
        if (!ret)
                goto out;
out:
        if (clut != NULL)
                g_ptr_array_unref (clut);
        return ret;
}


void ColorState::SessionDeviceAssignProfileConnectCb (GObject *object,
                                                      GAsyncResult *res,
                                                      gpointer user_data)
{
        CdProfile *profile = CD_PROFILE (object);
        const gchar *filename;
        gboolean ret;
        GError *error = NULL;
        MateRROutput *output;
        SessionAsyncHelper *helper = (SessionAsyncHelper *) user_data;
        ColorState *state = (ColorState *) (helper->state);

        /* get properties */
        ret = cd_profile_connect_finish (profile, res, &error);
        if (!ret) {
            if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            {
                qWarning ("failed to connect to profile: %s", error->message);
            }
            g_error_free (error);
            goto out;
        }

        /* get the filename */
        filename = cd_profile_get_filename (profile);
        g_assert (filename != NULL);

        /* get the output (can't save in helper as GnomeRROutput isn't
         * a GObject, just a pointer */
        output = mate_rr_screen_get_output_by_id (state->state_screen,
                                                   helper->output_id);
        if (output == NULL)
                goto out;

        /* set the _ICC_PROFILE atom */
        ret = SessionUseOutputProfileForScreen (state, output);
        if (ret) {
                ret = SessionScreenSetIccProfile (state,
                                                  filename,
                                                  &error);
                if (!ret) {
                        qWarning ("failed to set screen _ICC_PROFILE: %s",
                                   error->message);
                        g_clear_error (&error);
                }
        }

        /* create a vcgt for this icc file */
        ret = cd_profile_get_has_vcgt (profile);
        if (ret) {
                ret = SessionDeviceSetGamma (output,
                                             profile,
                                             state->color_temperature);
                if (!ret) {
                        qWarning ("failed to set %s gamma tables",
                                   cd_device_get_id (helper->device));
                        goto out;
                }
        } else {
                ret = SessionDeviceResetGamma (output,
                                               state->color_temperature);
                if (!ret) {
                        qWarning ("failed to reset %s gamma tables",
                                   cd_device_get_id (helper->device));
                        goto out;
                }
        }
out:
        SessionAsyncHelperFree (helper);
}


void ColorState::SessionDeviceAssignConnectCb (GObject *object,
                                               GAsyncResult *res,
                                               gpointer user_data)
{
    CdDeviceKind kind;
    CdProfile *profile = NULL;
    gboolean ret;
    gchar *autogen_filename = NULL;
    gchar *autogen_path = NULL;
    ColorEdid *edid = nullptr;
    MateRROutput *output = NULL;
    GError *error = NULL;
    GFile *file = NULL;
    const gchar *xrandr_id;
    SessionAsyncHelper *helper;
    CdDevice *device = CD_DEVICE (object);
    ColorState *state = (ColorState *)user_data;

    /* remove from assign array */
    g_hash_table_remove (state->device_assign_hash,
                     cd_device_get_object_path (device));

    /* get properties */
    ret = cd_device_connect_finish (device, res, &error);
    if (!ret) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
                qWarning ("failed to connect to device: %s", error->message);
        }
        g_error_free (error);
        goto out;
    }

    /* check we care */
    kind = cd_device_get_kind (device);
    if (kind != CD_DEVICE_KIND_DISPLAY)
        goto out;

    qDebug ("need to assign display device %s",
         cd_device_get_id (device));

    /* get the GnomeRROutput for the device id */
    xrandr_id = cd_device_get_id (device);
    output = SessionGetStateOutputById (state,
                                        xrandr_id);
    if (output == NULL) {
        qWarning ("no %s device found",
                   cd_device_get_id (device));
        goto out;
    }

    /* create profile from device edid if it exists */
    edid = SessionGetOutputEdid (state, output);
    if (edid == nullptr) {
        qWarning ("unable to get EDID for %s",
                   cd_device_get_id (device));

    } else {
        autogen_filename = g_strdup_printf ("edid-%s.icc",
                                            edid->EdidGetChecksum ());
        autogen_path = g_build_filename (g_get_user_data_dir (),
                                         "icc", autogen_filename, NULL);

        /* check if auto-profile has up-to-date metadata */
        file = g_file_new_for_path (autogen_path);
        if (SessionCheckProfileDeviceMd (file)) {
                qDebug ("auto-profile edid %s exists with md", autogen_path);
        } else {
                qDebug ("auto-profile edid does not exist, creating as %s",
                         autogen_path);
                /* check if the system has a built-in profile */
                ret = mate_rr_output_is_laptop (output) &&
                      GetSystemIccProfile (state, file);

                /* try creating one from the EDID */
                if (!ret) {
                        ret = ApplyCreateIccProfileForEdid (state,
                                                            device,
                                                            edid,
                                                            file,
                                                            &error);
                }

                if (!ret) {
                        qWarning ("failed to create profile from EDID data: %s",
                                     error->message);
                        g_clear_error (&error);
                }
        }
    }

    /* get the default profile for the device */
    profile = cd_device_get_default_profile (device);
    if (profile == NULL) {
        qDebug ("%s has no default profile to set",
                 cd_device_get_id (device));

        /* the default output? */
        if (mate_rr_output_get_is_primary (output) &&
            state->gdk_window != NULL) {
                gdk_property_delete (state->gdk_window,
                                     gdk_atom_intern_static_string ("_ICC_PROFILE"));
                gdk_property_delete (state->gdk_window,
                                     gdk_atom_intern_static_string ("_ICC_PROFILE_IN_X_VERSION"));
        }

        /* reset, as we want linear profiles for profiling */
        ret = SessionDeviceResetGamma (output,
                                       state->color_temperature);
        if (!ret) {
                qWarning ("failed to reset %s gamma tables",
                           cd_device_get_id (device));
                goto out;
        }
        goto out;
    }

    /* get properties */
    helper = g_new0 (SessionAsyncHelper, 1);
    helper->output_id = mate_rr_output_get_id (output);
    if(!helper->state)
        helper->state = state;
    helper->device = device;
    cd_profile_connect (profile,
                    state->cancellable,
                    SessionDeviceAssignProfileConnectCb,
                    helper);
    out:
    g_free (autogen_filename);
    g_free (autogen_path);
    if (file != NULL)
        g_object_unref (file);
    if (edid != nullptr)
        edid = nullptr;
    if (profile != NULL)
        g_object_unref (profile);
}


void ColorState::SessionProfileGammaFindDeviceCb (GObject *object,
                                          GAsyncResult *res,
                                          gpointer user_data)
{
        CdClient *client = CD_CLIENT (object);
        CdDevice *device = NULL;
        GError *error = NULL;
        ColorState *state = (ColorState *) user_data;

        device = cd_client_find_device_by_property_finish (client,
                                                           res,
                                                           &error);
        if (device == NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                        qWarning ("could not find device: %s", error->message);
                g_error_free (error);
                return;
        }

        /* get properties */
        cd_device_connect (device,
                           state->cancellable,
                           SessionDeviceAssignConnectCb,
                           state);

        if (device != NULL)
                g_object_unref (device);
}


void ColorState::SessionSetGammaForAllDevices (ColorState *state)
{
        MateRROutput **outputs;
        guint i;

        /* setting the temperature before we get the list of devices is fine,
         * as we use the temperature in the calculation */
        if (state->state_screen == NULL)
                return;

        /* get STATE outputs */
        outputs = mate_rr_screen_list_outputs (state->state_screen);
        if (outputs == NULL) {
                qWarning ("failed to get outputs");
                return;
        }
        for (i = 0; outputs[i] != NULL; i++) {
                /* get CdDevice for this output */
                cd_client_find_device_by_property (state->client,
                                                   CD_DEVICE_METADATA_XRANDR_NAME,
                                                   mate_rr_output_get_name (outputs[i]),
                                                   state->cancellable,
                                                   SessionProfileGammaFindDeviceCb,
                                                   state);
        }
}


/* We have to reset the gamma tables each time as if the primary output
 * has changed then different crtcs are going to be used.
 * See https://bugzilla.gnome.org/show_bug.cgi?id=660164 for an example */
void ColorState::MateRrScreenOutputChangedCb (MateRRScreen *screen,
                                              ColorState *state)
{
        SessionSetGammaForAllDevices (state);
}

void ColorState::SessionDeviceAssign (ColorState *state, CdDevice *device)
{
        const gchar *key;
        gpointer found;

        /* are we already assigning this device */
        key = cd_device_get_object_path (device);
        found = g_hash_table_lookup (state->device_assign_hash, key);
        if (found != NULL) {
                qDebug ("assign for %s already in progress", key);
                fprintf(stderr, "assign for %s already in progress\n", key);
                return;
        }
        g_hash_table_insert (state->device_assign_hash,
                             g_strdup (key),
                             GINT_TO_POINTER (TRUE));
        cd_device_connect (device,
                           state->cancellable,
                           SessionDeviceAssignConnectCb,
                           state);
}

void ColorState::SessionDeviceAddedAssignCb (CdClient *client,
                                             CdDevice *device,
                                             ColorState *state)
{
        SessionDeviceAssign (state, device);
}
void ColorState::SessionDeviceChangedAssignCb (CdClient *client,
                                               CdDevice *device,
                                               ColorState *state)
{
        qDebug ("%s changed", cd_device_get_object_path (device));
        SessionDeviceAssign (state, device);
}


void ColorState::SessionGetDevicesCb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        CdDevice *device;
        GError *error = NULL;
        GPtrArray *array;
        guint i;
        ColorState *state = (ColorState *)user_data;

        array = cd_client_get_devices_finish (CD_CLIENT (object), res, &error);
        if (array == NULL) {
                if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                        qWarning ("failed to get devices: %s", error->message);
                g_error_free (error);
                return;
        }
        for (i = 0; i < array->len; i++) {
                device = (CdDevice *)g_ptr_array_index (array, i);
                SessionDeviceAssign (state, device);
        }

        if (array != NULL)
                g_ptr_array_unref (array);
}

void ColorState::SessionClientConnectCb (GObject *source_object,
                                         GAsyncResult *res,
                                         gpointer user_data)
{
    gboolean ret;
    GError *error = NULL;
    MateRROutput **outputs;
    guint i;
    ColorState *state = (ColorState *)user_data;

    /* connected */
    ret = cd_client_connect_finish (state->client, res, &error);
    if (!ret) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            qWarning ("failed to connect to colord: %s", error->message);
        }
        g_error_free (error);
        return;
    }
    qDebug("success to connect to colord\n");
    /* is there an available colord instance? */
    ret = cd_client_get_has_server (state->client);
    if (!ret) {
        qWarning ("There is no colord server available");
        return;
    }

    /* add screens */
    mate_rr_screen_refresh (state->state_screen, &error);
    if (error != NULL) {
        qWarning ("failed to refresh: %s", error->message);
        g_error_free (error);
        return;
    }

    /* get STATE outputs */
    outputs = mate_rr_screen_list_outputs (state->state_screen);
    if (outputs == NULL) {
        qWarning ("failed to get outputs");
        return;
    }
    for (i = 0; outputs[i] != NULL; i++) {
        SessionAddStateOutput (state, outputs[i]);
    }

    /* only connect when colord is awake */
    g_signal_connect (state->state_screen, "changed",
                      G_CALLBACK (MateRrScreenOutputChangedCb),
                      state);
    g_signal_connect (state->client, "device-added",
                      G_CALLBACK (SessionDeviceAddedAssignCb),
                      state);
    g_signal_connect (state->client, "device-changed",
                      G_CALLBACK (SessionDeviceChangedAssignCb),
                      state);

    /* set for each device that already exist */
    cd_client_get_devices (state->client,
                           state->cancellable,
                           SessionGetDevicesCb,
                           state);
}

bool ColorState::ColorStateStart()
{
    g_cancellable_cancel (cancellable);
    g_clear_object (&cancellable);
    cancellable = g_cancellable_new ();
    gdk_init(NULL,NULL);
    GError *error = NULL;
    state_screen = mate_rr_screen_new(gdk_screen_get_default (), &error);
    if (state_screen == NULL) {
            qWarning ("failed to get screens: %s", error->message);
            g_error_free (error);
            return false;
    }

    cd_client_connect (client,
                       cancellable,
                       SessionClientConnectCb,
                       this);
    return true;
}
void ColorState::ColorStateStop()
{
    g_cancellable_cancel (cancellable);
}
