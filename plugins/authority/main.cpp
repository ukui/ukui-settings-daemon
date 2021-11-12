#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include "authorityservice.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    a.setOrganizationName("Kylin Team");

    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (systemBus.registerService("org.ukui.authority")){
        systemBus.registerObject("/", new AuthorityService(),
                                 QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals
                                 );
    }

    return a.exec();
}
