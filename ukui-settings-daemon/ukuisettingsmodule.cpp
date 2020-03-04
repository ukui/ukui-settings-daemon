#include "ukuisettingsmodule.h"
#include <syslog.h>

UkuiSettingsModule::UkuiSettingsModule(QString path)
{
//    syslog(LOG_DEBUG, "Creating object of type %s", g_type_name(this->type));
//    if (0 == this->type) {
//        syslog(LOG_ERR, "module type is 0");
//        return;
//    }
    if (path.isNull() || path.isEmpty()) {
        syslog(LOG_WARNING, "module path is not valid!");
        return;
    }
    syslog(LOG_DEBUG, "Creating a new module %s", (char*)path.data());

    // 设置名字 g_type_module_set_name
    getModuleName(path);

    // 保存路径
    this->path = path;
}

QString UkuiSettingsModule::ukuiSettingsModuleGetPath()
{
    return this->path;
}

// FIXME://
// 暂时不实现此功能
void UkuiSettingsModule::getModuleName(QString& path)
{
    this->type = path;
}
