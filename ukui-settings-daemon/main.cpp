#include <iostream>
#include <syslog.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "ukuisettingsmanager.h"

//using namespace std;

int main()
{
    UkuiSettingsManager*    manager = NULL;
    DBusGConnection*        bus = NULL;


    std::cout << "Hello World!" << std::endl;
    return 0;
}
