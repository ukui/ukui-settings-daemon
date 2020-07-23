#ifndef __USD_INPUT_HELPER_H
#define __USD_INPUT_HELPER_H

#include <glib.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

gboolean  supports_xinput_devices (void);
XDevice  *device_is_touchpad      (XDeviceInfo *deviceinfo);
gboolean  touchpad_is_present     (void);



#endif /* __USD_INPUT_HELPER_H */
