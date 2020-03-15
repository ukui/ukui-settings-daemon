#include "xsettings.h"

PluginInterface* Xsettings::m_pXsettings = nullptr;

PluginInterface* Xsettings::getInstance()
{
    if (nullptr == Xsettings::m_pXsettings) {
        Xsettings::m_pXsettings = new Xsettings();
    }
    return Xsettings::m_pXsettings;
}


PluginInterface* createSettingsPlugin() {
    return Xsettings::getInstance();
}


Xsettings::Xsettings()
{
    m_pXsettingManager = new ukuiXSettingsManager();
}


Xsettings::~Xsettings()
{
    if (m_pXsettingManager)
        delete m_pXsettingManager;
    m_pXsettingManager = nullptr;
}


void Xsettings::activate()
{
    gboolean res;
    GError  *error;

    g_debug ("Activating xsettings plugin");

    error = NULL;
    m_pXsettingManager->start(&error);
    if (! res) {
        g_warning ("Unable to start xsettings manager: %s", error->message);
        g_error_free (error);
    }
}

void Xsettings::deactivate()
{
    m_pXsettingManager->stop();
}


