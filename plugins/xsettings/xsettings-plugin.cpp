#include "xsettings-plugin.h"
#include <syslog.h>

PluginInterface* createSettingsPlugin() {
    return new Xsettings();
}


Xsettings::Xsettings()
{
    m_pukuiXsettingManager = new ukuiXSettingsManager();
}


Xsettings::~Xsettings()
{
    if (m_pukuiXsettingManager)
        delete m_pukuiXsettingManager;
    m_pukuiXsettingManager = nullptr;
}


void Xsettings::activate()
{
    int res;
    GError  *error;

    g_debug ("Activating xsettings plugin");

    error = NULL;
    res = m_pukuiXsettingManager->start(&error);
    if (!res) {
        syslog(LOG_ERR, "false m_pukuiXsettingManager start");
        g_warning ("Unable to start xsettings manager: %s", error->message);
        g_error_free (error);
    }
    syslog(LOG_ERR, "END PLUGIN activate");
}

void Xsettings::deactivate()
{
    syslog(LOG_ERR, "begin PLUGIN deactivate");
    m_pukuiXsettingManager->stop();
    syslog(LOG_ERR, "end PLUGIN deactivate");
}


