#ifndef SHARINGDBUS_H
#define SHARINGDBUS_H

#include <QObject>

class sharingDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.Sharing")

public:
    sharingDbus(QObject* parent=0);
    ~sharingDbus();

public Q_SLOTS:
    void EnableService(QString serviceName);
    void DisableService(QString serviceName);

Q_SIGNALS:
    void serviceChange(QString state, QString serviceName);

};

#endif // SHARINGDBUS_H
