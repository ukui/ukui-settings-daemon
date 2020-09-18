#include <QDebug>
#include "color-plugin.h"

PluginInterface *ColorPlugin::mInstance      = nullptr;
ColorManager    *ColorPlugin::mColorManager  = nullptr;

ColorPlugin::ColorPlugin()
{
    qDebug()<<"Color Plugin initializing";
    if(nullptr == mColorManager)
        mColorManager = ColorManager::ColorManagerNew();
}

ColorPlugin::~ColorPlugin()
{
    if(mColorManager)
        delete mColorManager;
    if(mInstance)
        delete mInstance;
}
void ColorPlugin::activate()
{
    qDebug("activating Color plugins");
    bool res = mColorManager->ColorManagerStart();
    if(!res)
        qWarning("Unable to start Color manager!");
}

void ColorPlugin::deactivate()
{
    qDebug("Deactivating Color plugin");
    mColorManager->ColorManagerStop();
}

PluginInterface *ColorPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new ColorPlugin();

    return mInstance;
}

PluginInterface *createSettingsPlugin()
{
    return ColorPlugin::getInstance();
}

