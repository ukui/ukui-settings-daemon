#include "rfkillswitch.h"
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDBusConnection>


RfkillSwitch *RfkillSwitch::m_rfkillInstance = new RfkillSwitch();

bool RfkillSwitch::isVirtualWlan(QString dp){
    QDir dir(VIRTUAL_PATH);
    if (!dir.exists()){
        return false;
    }

    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::Name);

    int count = dir.count();
    if (count <= 0){
        return false;
    }

    QFileInfoList fileinfoList = dir.entryInfoList();

    for(QFileInfo fileinfo : fileinfoList){
        if (fileinfo.fileName() == "." || fileinfo.fileName() == ".."){
            continue;
        }

        if (QString::compare(fileinfo.fileName(), dp) == 0){
            return true;
        }
    }

    return false;

}

int RfkillSwitch::getCurrentWlanMode(){
    struct rfkill_event event;
    ssize_t len;
    int fd;
    int bls = 0, unbls = 0;

    QList<int> status;

    fd = open("/dev/rfkill", O_RDONLY);
    if (fd < 0) {
        qCritical("Can't open RFKILL control device");
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        qCritical("Can't set RFKILL control device to non-blocking");
        close(fd);
        return -1;
    }

    while (1) {
        len = read(fd, &event, sizeof(event));
        if (len < 0) {
            if (errno == EAGAIN)
                break;
            qWarning("Reading of RFKILL events failed");
            break;
        }

        if (len != RFKILL_EVENT_SIZE_V1) {
            qWarning("Wrong size of RFKILL event\n");
            continue;
        }

//        printf("%u - %u: %u\n", event.idx, event.type, event.soft);
        if (event.type != RFKILL_TYPE_WLAN)
            continue;

        if (isVirtualWlan(QString(getRFkillName(event.idx)))){
            continue;
        }


        status.append(event.soft ? 1 : 0);
    }

    close(fd);

    if (status.length() == 0){
        return -1;
    }

    for (int s : status){
        s ? bls++ : unbls++;
    }

    if (bls == status.length()){ //block
        return 0;
    } else if (unbls == status.length()){ //unblock
        return 1;
    } else { //not block & not unblock
        return 0;
    }
}

int RfkillSwitch::getCurrentFlightMode(){

    struct rfkill_event event;
    ssize_t len;
    int fd;
    int bls = 0, unbls = 0;

    QList<int> status;

    fd = open("/dev/rfkill", O_RDONLY);
    if (fd < 0) {
        qCritical("Can't open RFKILL control device");
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        qCritical("Can't set RFKILL control device to non-blocking");
        close(fd);
        return -1;
    }

    while (1) {
        len = read(fd, &event, sizeof(event));

        if (len < 0) {
            if (errno == EAGAIN)
                break;
            qWarning("Reading of RFKILL events failed");
            break;
        }

        if (len != RFKILL_EVENT_SIZE_V1) {
            qWarning("Wrong size of RFKILL event\n");
            continue;
        }

        if (isVirtualWlan(QString(getRFkillName(event.idx)))){
            continue;
        }

//        printf("%u: %u\n", event.idx, event.soft);
        status.append(event.soft ? 1 : 0);

    }

    close(fd);

    if (status.length() == 0){
        return -1;
    }

    for (int s : status){
        s ? bls++ : unbls++;
    }

    if (bls == status.length()){ //block
        return 1;
    } else if (unbls == status.length()){ //unblock
        return 0;
    } else { //not block & not unblock
        return 0;
    }
}

QString RfkillSwitch::toggleFlightMode(bool enable){

    struct rfkill_event event;
    int fd;
    ssize_t len;

    __u8 block;

    fd = open("/dev/rfkill", O_RDWR);
    if (fd < 0) {
        return QString("Can't open RFKILL control device");
    }

    block = enable ? 1 : 0;

    memset(&event, 0, sizeof(event));

    event.op= RFKILL_OP_CHANGE_ALL;

    /* RFKILL_TYPE_ALL = 0 */
    event.type = RFKILL_TYPE_ALL;

    event.soft = block;

    len = write(fd, &event, sizeof(event));

    if (len < 0){
        return QString("Failed to change RFKILL state");
    }

    close(fd);
    return block ? QString("block"):QString("unblock");
}


QString RfkillSwitch::getWifiState()
{

    if(!wifiDeviceIsPresent()){
        return "";
    }

    QString cmd  = "nmcli radio wifi";
    QProcess process;
    process.start(cmd);
    process.waitForStarted();
    process.waitForFinished();
    QString wifiState = QString::fromLocal8Bit(process.readAllStandardOutput());
    wifiState.replace("\n","");

    return wifiState;
}

bool RfkillSwitch::wifiDeviceIsPresent()
{

    QDBusInterface m_interface( "org.freedesktop.NetworkManager",
                              "/org/freedesktop/NetworkManager",
                              "org.freedesktop.NetworkManager",
                              QDBusConnection::systemBus() );

   //先获取所有的网络设备的设备路径
   QDBusReply<QList<QDBusObjectPath>> obj_reply = m_interface.call("GetAllDevices");
   if (!obj_reply.isValid()) {
       qDebug()<<"execute dbus method 'GetAllDevices' is invalid in func getObjectPath()";
       return false;
   }

   QList<QDBusObjectPath> obj_paths = obj_reply.value();

   //再判断有无有线设备和无线设备的路径
   Q_FOREACH (QDBusObjectPath obj_path, obj_paths) {
       QDBusInterface interface( "org.freedesktop.NetworkManager",
                                 obj_path.path(),
                                 "org.freedesktop.DBus.Introspectable",
                                 QDBusConnection::systemBus() );

       QDBusReply<QString> reply = interface.call("Introspect");
       if (reply.isValid()) {
           if (reply.value().indexOf("org.freedesktop.NetworkManager.Device.Wireless") != -1) {
               //表明有wifi设备
               return true;
           }
       }
   }
   return false;
}

void RfkillSwitch::turnWifiOn()
{
    QString cmd = "nmcli radio wifi on";
    QProcess::execute(cmd);
}

void RfkillSwitch::turnWifiOff()
{
    QString cmd = "nmcli radio wifi off";
    QProcess::execute(cmd);
}

//Camera

QString RfkillSwitch::getCameraBusinfo(){

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
                if (output.contains("camera", Qt::CaseInsensitive)){
                    return fileinfo.fileName();
                }

            }
        }

    }

    return QString();
}


int RfkillSwitch::getCameraDeviceEnable(){

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
            //no Camera Device
            ret = 0;
        } else {
            //Camera Device Present
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

QString RfkillSwitch::toggleCameraDevice(QString businfo){

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

int RfkillSwitch::getCurrentBluetoothMode()
{
    struct rfkill_event event;
    ssize_t len;
    int fd;
    int bls = 0, unbls = 0;

    QList<int> status;

    fd = open("/dev/rfkill", O_RDONLY);
    if (fd < 0) {
        qCritical("Can't open RFKILL control device");
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        qCritical("Can't set RFKILL control device to non-blocking");
        close(fd);
        return -1;
    }

    while (1) {
        len = read(fd, &event, sizeof(event));
        if (len < 0) {
            if (errno == EAGAIN)
                break;
            qWarning("Reading of RFKILL events failed");
            break;
        }

        if (len != RFKILL_EVENT_SIZE_V1) {
            qWarning("Wrong size of RFKILL event\n");
            continue;
        }

//        printf("%u - %u: %u\n", event.idx, event.type, event.soft);
        if (event.type != RFKILL_TYPE_BLUETOOTH)
            continue;

        status.append(event.soft ? 1 : 0);
    }

    close(fd);

    if (status.length() == 0){
        return -1;
    }

    for (int s : status){
        s ? bls++ : unbls++;
    }

    if (bls == status.length()){ //block
        return 0;
    } else if (unbls == status.length()){ //unblock
        return 1;
    } else { //not block & not unblock
        return 0;
    }
}

QString RfkillSwitch::toggleBluetoothMode(bool enable)
{
    struct rfkill_event event;
    int fd;
    ssize_t len;

    __u8 block;

    fd = open("/dev/rfkill", O_RDWR);
    if (fd < 0) {
        return QString("Can't open RFKILL control device");
    }

    block = enable ? 0 : 1;

    memset(&event, 0, sizeof(event));

    event.op= RFKILL_OP_CHANGE_ALL;

    event.type = RFKILL_TYPE_BLUETOOTH;

    event.soft = block;

    len = write(fd, &event, sizeof(event));

    if (len < 0){
        return QString("Failed to change RFKILL state");
    }

    close(fd);
    return block ? QString("blocked") : QString("unblocked");
}


const char * getRFkillName(__u32 idx){

    static char name[128] = {};
    char *pos, filename[64];
    int fd;

    snprintf(filename, sizeof(filename) - 1,
             "/sys/class/rfkill/rfkill%u/name", idx);

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    memset(name, 0, sizeof(name));
    read(fd, name, sizeof(name) - 1);

    pos = strchr(name, '\n');
    if (pos)
        *pos = '\0';

    close(fd);

    return name;
}

const char * getRFkillType(__u32 idx){

    static char name[128] = {};
    char *pos, filename[64];
    int fd;

    snprintf(filename, sizeof(filename) - 1,
                "/sys/class/rfkill/rfkill%u/type", idx);

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    memset(name, 0, sizeof(name));
    read(fd, name, sizeof(name) - 1);

    pos = strchr(name, '\n');
    if (pos)
        *pos = '\0';

    close(fd);

    return name;
}


