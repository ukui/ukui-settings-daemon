#include "xsettings.h"
#include "xsettings-manager.h"

PluginInterface* Xsettings::mXsettings = nullptr;

PluginInterface* Xsettings::getInstance()
{
    if (nullptr == mXsettings) {
        mXsettings = new Xsettings();
    }
    return mXsettings;
}

Xsettings::Xsettings()
{
//        mXsettingManager = new XsettingsManager();
}

Xsettings::~Xsettings()
{
//    if(mXsettingManager != nullptr)
//        delete mXsettingManager;
//    mXsettingManager = nullptr;
}


void Xsettings::activate()
{
//    mXsettingManager->start();
}

void Xsettings::deactivate()
{
//    mXsettingManager->stop();
}

PluginInterface* createSettingsPlugin()
{
    return Xsettings::getInstance();
}
