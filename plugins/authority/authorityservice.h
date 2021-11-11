#ifndef AUTHORITYSERVICE_H
#define AUTHORITYSERVICE_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QTextStream>

class AuthorityService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ukui.authority.interface")

public:
    explicit AuthorityService(QObject *parent = nullptr);
    ~AuthorityService(){}
public slots:

    Q_SCRIPTABLE QString getCameraBusinfo();
    Q_SCRIPTABLE int getCameraDeviceEnable();
    Q_SCRIPTABLE QString toggleCameraDevice(QString businfo);
    Q_SCRIPTABLE int setCameraKeyboardLight(bool lightup);

signals:

};

#endif // AUTHORITYSERVICE_H
