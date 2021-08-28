#include "rfkillswitch.h"
#include <QDir>
#include <QFile>
#include <QTextStream>


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
