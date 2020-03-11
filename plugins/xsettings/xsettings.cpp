#include "xsettings.h"
#include "xsettings-manager.h"

PluginInterface* Xsettings::getInstance()
{
    return nullptr;
}

Xsettings::Xsettings()
{
        pXsettingManager = new XsettingsManager();
}

Xsettings::~Xsettings()
{
    if(pXsettingManager != nullptr)
        delete pXsettingManager;
    pXsettingManager = nullptr;
}


void Xsettings::activate()
{
    pXsettingManager->start();
}

void Xsettings::deactivate()
{
    pXsettingManager->stop();
}
