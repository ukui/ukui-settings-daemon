#include "ukuisettingsplugin.h"

#include <QDebug>
#include <QCoreApplication>
#include <QLibrary>

int main(void)
{
    QLibrary lib("/home/dingjing/q/libbackground");
    if (lib.load()) {
        // 定义插件中两个导出函数的原型
        typedef UkuiSettingsPlugin* (*createPlugin) ();
        createPlugin p = (createPlugin)lib.resolve("createSettingsPlugin");
        if (NULL != p) {
            UkuiSettingsPlugin* pl = p();
            pl->activate();

        } else {
            qDebug() << "parse error: " << lib.errorString();
        }
    } else {
        qDebug() << "error: " << lib.errorString();
    }

    // 卸载插件
    lib.unload();
    return 0;
}
