#include "clipboard-plugin.h"
#include "clib-syslog.h"

ClipboardManager* ClipboardPlugin::mManager = nullptr;
PluginInterface* ClipboardPlugin::mInstance = nullptr;

ClipboardPlugin::~ClipboardPlugin()
{
    delete mManager;
    mManager = nullptr;
}

PluginInterface *ClipboardPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new ClipboardPlugin();
    }
    return mInstance;
}

void ClipboardPlugin::activate()
{
    mManager->start();
}

void ClipboardPlugin::deactivate()
{
    mManager->stop();
    if (nullptr != mInstance) {
        delete mInstance;
        mInstance = nullptr;
    }
}

ClipboardPlugin::ClipboardPlugin()
{
    syslog_init("ukui-settings-daemon-clipboard", LOG_LOCAL6);
    if (nullptr == mManager) {
        mManager = new ClipboardManager();
    }
}

PluginInterface* createSettingsPlugin()
{
    return ClipboardPlugin::getInstance();
}

