#ifndef XINPUTMANAGER_H
#define XINPUTMANAGER_H

#include <QObject>
#include <QThread>

#include "monitorinputtask.h"

extern "C"{
    #include "clib-syslog.h"
}

class QGSettings;
class XinputManager : public QObject
{
    Q_OBJECT
public:
    XinputManager(QObject *parent = nullptr);
    ~XinputManager();

    void start();
    void stop();

Q_SIGNALS:
    void sigStartThread();

private:
    void init();
    bool deviceHasProperty(XDevice    *device,
                           const char *property_name);
private:
    QThread *m_pManagerThread;
    QMutex m_runningMutex;
    MonitorInputTask *m_pMonitorInputTask;
    QGSettings* m_settings;

private Q_SLOTS:
    void onSlaveAdded(int device_id);
    void updateSettings(QString key);

    /*!
     * \brief update tablet tool button map
     * It will refresh when the gsetting updates
     * or receives an increased signal from the device
    */
    void updateButtonMap();

private:
    void SetPenRotation(int device_id);
    QQueue<int> GetPenDevice();
    void initSettings();
};

#endif // XINPUTMANAGER_H
