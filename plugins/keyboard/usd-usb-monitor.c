#include <stdio.h>
#include <stdlib.h>
#include <string.h>;
#include <glib.h>
#include <libusb-1.0/libusb.h>
#include <stdbool.h>

static int
LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    sleep(1);
    system("xmodmap -e 'add mod3 = Scroll_Lock'");

    return 0;
}

int main (int argc, char *argv[])
{

        libusb_hotplug_callback_handle hp[2];
        int product_id, vendor_id, class_id;
        int rc;
        char buf1[512]="";
        bool flag = true;
        vendor_id  =  LIBUSB_HOTPLUG_MATCH_ANY;
        product_id =  LIBUSB_HOTPLUG_MATCH_ANY;
        class_id   =  LIBUSB_HOTPLUG_MATCH_ANY;
        
        rc = libusb_init (NULL);
        if (rc < 0) {
            printf("failed to initialise libusb: %s\n", libusb_error_name(rc));
            return EXIT_FAILURE;
        }
        if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
            printf ("Hotplug capabilites are not supported on this platform\n");
            libusb_exit (NULL);
            return EXIT_FAILURE;
        }
        rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
                                               product_id, class_id, hotplug_callback, NULL, &hp[0]);
        if (LIBUSB_SUCCESS != rc) {
            fprintf (stderr, "Error registering callback 0\n");
        }

        sleep(1);
        system("xmodmap -e 'add mod3 = Scroll_Lock'");

        while (1) {
            rc = libusb_handle_events (NULL);
            if (rc < 0)
                printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));
        }

        libusb_exit (NULL);

        return EXIT_SUCCESS;
}
