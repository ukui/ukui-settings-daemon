#include "ukuisettingsplugin.h"

#include <QDebug>
#include <QCoreApplication>
#include <QLibrary>

/*
    mModule = new QLibrary(path);
    if (!(mModule->load())) {
        CT_SYSLOG(LOG_ERR, "create module '%s' error!", path.toUtf8().data());
        mAvailable = false;
        return false;
    }
    typedef UkuiSettingsPlugin* (*createPlugin) ();
    createPlugin p = (createPlugin)mModule->resolve("createSettingsPlugin");

  */
int main(void)
{
    QLibrary lib("/home/dingjing/q/libbackground");
    if (lib.load()) {
        // 定义插件中两个导出函数的原型
        typedef UkuiSettingsPlugin* (*createPlugin) ();
        createPlugin p = (createPlugin)lib.resolve("createSettingsPlugin");

        // 解析导出函数
//        CreateAnimalFunction createAnimal = (CreateAnimalFunction) lib.resolve("CreateAnimal");
//        ReleaseAnimalFunction releaseAnimal = (ReleaseAnimalFunction) lib.resolve("ReleaseAnimal");

//        if (createAnimal && releaseAnimal) {
//            // 创建 Animal 对象
//            IAnimal* animal = createAnimal();
//            if (animal) {
//                // 使用插件功能
//                animal->eat();
//                animal->sleep();

//                // 使用完之后删除对象
//                releaseAnimal(animal);
//            }
        } else {
            qDebug() << "parse error: " << lib.errorString();
        }
//    } else {
//        qDebug() << "error: " << lib.errorString();
//    }

    // 卸载插件
    lib.unload();
    return 0;
}
