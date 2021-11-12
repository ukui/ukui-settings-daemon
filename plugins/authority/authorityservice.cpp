#include "authorityservice.h"

extern "C" {

#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
}

AuthorityService::AuthorityService(QObject *parent) : QObject(parent)
{

}

QString AuthorityService::getCameraBusinfo()
{

    QString path = QString("/sys/bus/usb/devices/");
    QDir dir(path);
    if (!dir.exists()){
        return QString();
    }
    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::Name);
    QFileInfoList fileinfoList = dir.entryInfoList();

    for(QFileInfo fileinfo : fileinfoList){
        if (fileinfo.fileName() == "." || fileinfo.fileName() == ".."){
            continue;
        }

        if (fileinfo.fileName().contains(":")){
            continue;
        }

        if (fileinfo.fileName().startsWith("usb")){
            continue;
        }
//        qDebug() << "" << fileinfo.fileName() << fileinfo.absoluteFilePath();
        QDir subdir(fileinfo.absoluteFilePath());
        subdir.setFilter(QDir::Files);
        QFileInfoList fileinfoList2 = subdir.entryInfoList();
        for (QFileInfo fileinfo2 : fileinfoList2){
            if (fileinfo2.fileName() == "product"){
                QFile pfile(fileinfo2.absoluteFilePath());
                if (!pfile.open(QIODevice::ReadOnly | QIODevice::Text))
                    return QString();
                QTextStream pstream(&pfile);
                QString output = pstream.readAll();
//                qDebug() << "output: " << output;
                if (output.contains("camera", Qt::CaseInsensitive)){
                    return fileinfo.fileName();
                }

            }
        }

    }

    return QString();
}

int AuthorityService::getCameraDeviceEnable()
{

    QString businfo = getCameraBusinfo();
    if (businfo.isEmpty()){
        char * cmd = "lsusb -t | grep 'Driver=uvcvideo'";
        char output[1024] = "\0";

        FILE * stream;
        if ((stream = popen(cmd, "r")) == NULL){
            return -1;
        }

    int ret;
        if (fread(output, sizeof(char), 1024, stream) <= 0){
        ret = 0;
        } else {
        ret = 1;
        }
    fclose(stream);
    return ret;
    }

    int isExists = 0;

    QString path = QString("/sys/bus/usb/drivers/usb/");
    QDir dir(path);
    if (!dir.exists()){
        return -1;
    }
    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::Name);
    QFileInfoList fileinfoList = dir.entryInfoList();

    for(QFileInfo fileinfo : fileinfoList){
        if (fileinfo.fileName() == "." || fileinfo.fileName() == ".."){
            continue;
        }

        if (fileinfo.fileName().contains(":")){
            continue;
        }

        if (fileinfo.fileName() == businfo){
            isExists = 1;
        }
    }

    return isExists;
}

QString AuthorityService::toggleCameraDevice(QString businfo)
{

    QString path = QString("/sys/bus/usb/drivers/usb/");

    int status = getCameraDeviceEnable();

    if (status == -1){
        return QString("Camera Device Not Exists!");
    }

    if (status){
        QString cmd = QString("echo '%1' > %2/unbind").arg(businfo).arg(path);
        system(cmd.toLatin1().data());
        return QString("unbinded");
    } else {
        QString cmd = QString("echo '%1' > %2/bind").arg(businfo).arg(path);
        system(cmd.toLatin1().data());
        return QString("binded");
    }
}

int AuthorityService::setCameraKeyboardLight(bool lightup)
{
    char * target = "/sys/class/leds/platform::cameramute/brightness";
    if (access(target, F_OK) == -1){
        return -1;
    }

    int value = lightup ? 1 : 0;

    char cmd[256];
    sprintf(cmd, "echo %d > %s", value, target);

    system(cmd);

    return 1;
}
