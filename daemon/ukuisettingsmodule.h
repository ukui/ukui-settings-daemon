#ifndef UKUISETTINGSMODULE_H
#define UKUISETTINGSMODULE_H
#include <QString>
#include <glib-object.h>

class UkuiSettingsModule
{
public:
    UkuiSettingsModule(QString path);

    QString ukuiSettingsModuleGetPath();

private:
    void getModuleName(QString& path);

private:
    QString path;                       // 动态模块路径
    QString type;                       // 唯一的ID, 也就是插件名字
};

#endif // UKUISETTINGSMODULE_H
