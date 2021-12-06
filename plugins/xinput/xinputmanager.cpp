#include "xinputmanager.h"

#include <QProcess>
#include <QQueue>
#include <QGSettings/QGSettings>

#define UKUI_CONTROL_CENTER_PEN_SCHEMA    "org.ukui.control-center.pen"
#define RIGHT_CLICK_KEY                   "right-click"

XinputManager::XinputManager(QObject *parent):
    QObject(parent)
{
    init();
}

void XinputManager::init()
{
    // init device monitor
    m_pMonitorInputTask = MonitorInputTask::instance();
    connect(this, &XinputManager::sigStartThread, m_pMonitorInputTask, &MonitorInputTask::StartManager);
    connect(m_pMonitorInputTask, &MonitorInputTask::slaveAdded, this, &XinputManager::onSlaveAdded);
    connect(m_pMonitorInputTask, &MonitorInputTask::slaveAdded, this, &XinputManager::updateButtonMap);

    m_pManagerThread = new QThread(this);
    m_pMonitorInputTask->moveToThread(m_pManagerThread);

    // init pen settings
    if ( false == initSettings()) {
        return ;
    }

    // init settings monitor
    connect(m_penSettings, SIGNAL(changed(QString)), this, SLOT(updateSettings(QString)));
}

XinputManager::~XinputManager()
{
    if (m_penSettings)
        delete m_penSettings;
}

void XinputManager::start()
{
    USD_LOG(LOG_DEBUG,"info: [XinputManager][StartManager]: thread id =%d",QThread::currentThreadId());
    m_runningMutex.lock();
    m_pMonitorInputTask->m_running = true;
    m_runningMutex.unlock();

    m_pManagerThread->start();
    Q_EMIT sigStartThread();
}

void XinputManager::stop()
{
    if(m_pManagerThread->isRunning())
    {
        m_runningMutex.lock();
        m_pMonitorInputTask->m_running = false;
        m_runningMutex.unlock();

        m_pManagerThread->quit();
    }
}

void XinputManager::SetPenRotation(int device_id)
{
    Q_UNUSED(device_id);
    // 得到触控笔的ID
    QQueue<int> penDeviceQue;
    int  ndevices = 0;
    Display *dpy = XOpenDisplay(NULL);
    XIDeviceInfo *info = XIQueryDevice(dpy, XIAllDevices, &ndevices);

    for (int i = 0; i < ndevices; i++)
    {
        XIDeviceInfo* dev = &info[i];
        // 判断当前设备是不是触控笔
        if(dev->use != XISlavePointer) continue;
        if(!dev->enabled) continue;
        for (int j = 0; j < dev->num_classes; j++)
        {
            if (dev->classes[j]->type == XIValuatorClass)
            {
                XIValuatorClassInfo *t = (XIValuatorClassInfo*)dev->classes[j];
                // 如果当前的设备是绝对坐标映射 则认为该设备需要进行一次屏幕映射
                if (t->mode == XIModeAbsolute)
                {
                    penDeviceQue.enqueue(dev->deviceid);
                    break;
                }
            }
        }
    }
    if(!penDeviceQue.size())
    {
//        qDebug() << "info: [XrandrManager][SetPenRotation]: Do not neet to pen device map-to-output outDevice!";
        //return;
        goto FREE_DPY;
    }
    // 设置map-to-output
    while(penDeviceQue.size())
    {
        int pen_device_id = penDeviceQue.dequeue();
        QString name = "eDP-1"; //针对该项目而言，笔记本内显固定为 eDP-1
        QString cmd = QString("xinput map-to-output %1 %2").arg(pen_device_id).arg(name);
        QProcess::execute(cmd);
    }
FREE_DPY:
    XIFreeDeviceInfo(info);
    XCloseDisplay(dpy);
}

void XinputManager::onSlaveAdded(int device_id)
{
//    qDebug() << "info: [XinputManager][onSlaveAdded]: Slave Device(id =" << device_id << ") Added!";
    SetPenRotation(device_id);//新设备不是触控笔就不要处理了！
}


bool XinputManager::initSettings()
{
    if (!QGSettings::isSchemaInstalled(UKUI_CONTROL_CENTER_PEN_SCHEMA)) {
        USD_LOG(LOG_DEBUG,"Can not find schema org.ukui.control-center.pen!");
        return false;
    }

    m_penSettings = new QGSettings(UKUI_CONTROL_CENTER_PEN_SCHEMA);

    updateButtonMap();

    return true;
}

void XinputManager::updateSettings(QString key)
{
    if (key == RIGHT_CLICK_KEY) {
        updateButtonMap();
    }
}

void XinputManager::updateButtonMap()
{
    QQueue<int> deviceQue = GetPenDevice();

    if (!deviceQue.size()) {
        return;
    }

    QString command;

    //! \brief The button-map default value is 1234567,
    //! and the modified mapping is 1334567
    //! The numbers in this refer to button 2 being mapped to button 3
    while (deviceQue.size()) {
        if (m_penSettings->get(RIGHT_CLICK_KEY).value<bool>()) {
            command = QString("xinput set-button-map %1 1 3 3").arg(deviceQue.dequeue());
        }
        else {
            command = QString("xinput set-button-map %1 1 2 3").arg(deviceQue.dequeue());
        }
        QProcess::execute(command);
    }
}

QQueue<int> XinputManager::GetPenDevice()
{
    QQueue<int>   penDeviceQue;
    int           ndevices = 0;
    Display*      dpy = XOpenDisplay(NULL);
    XIDeviceInfo* info = XIQueryDevice(dpy, XIAllDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        XIDeviceInfo* dev = &info[i];

        if(dev->use != XISlavePointer)
            continue;

        if(!dev->enabled)
            continue;

        XDevice* device = XOpenDevice(dpy, dev->deviceid);

        /*!
         * \note Currently only pen devices have this property,
         * but it is not certain that this property must be pen-only
        */
        if (deviceHasProperty(device, "libinput Tablet Tool Pressurecurve")) {
            penDeviceQue.enqueue(dev->deviceid);
        }

        XCloseDevice(dpy, device);
    }

    XIFreeDeviceInfo(info);
    XCloseDisplay(dpy);

    return penDeviceQue;
}

bool XinputManager::deviceHasProperty(XDevice    *device,
                                      const char *property_name)
{
    Display*       dpy = XOpenDisplay(NULL);
    Atom           realtype, prop;
    int            realformat;
    unsigned long  nitems, bytes_after;
    unsigned char* data;

    prop = XInternAtom (dpy, property_name, True);
    if (!prop) {
        XCloseDisplay(dpy);
        return false;
    }

    try {
        if ((XGetDeviceProperty(dpy, device, prop, 0, 1, False,
                                XA_INTEGER, &realtype, &realformat, &nitems,
                                &bytes_after, &data) == Success) && (realtype != None)) {
            XFree (data);
            XCloseDisplay(dpy);
            return true;
        }
    } catch (int x) {

    }

    XCloseDisplay(dpy);
    return false;
}
