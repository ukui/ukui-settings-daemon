#ifndef MPRISMANAGER_H
#define MPRISMANAGER_H

#include <QObject>
#include <QQueue>
#include <QString>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusConnection>

/** undef 'signals' from qt,avoid conflict with Glib
 *  undef qt中的 'signals' 关键字，避免与 Glib 冲突
 */
#ifdef signals
#undef signals
#endif

#include <gio/gio.h>        //for GError

class MprisManager : public QObject{
    Q_OBJECT
public:
    ~MprisManager();
    static MprisManager* MprisManagerNew();
    bool MprisManagerStart(GError **error);
    void MprisManagerStop();

private:
    MprisManager(QObject *parent = nullptr);
    MprisManager(const MprisManager&) = delete;

private Q_SLOTS:
    void serviceRegisteredSlot(const QString&);
    void serviceUnregisteredSlot(const QString&);
    void keyPressed(QString,QString);

private:
    static MprisManager   *mMprisManager;
    QDBusServiceWatcher   *mDbusWatcher;
    QDBusInterface        *mDbusInterface;
    QQueue<QString>       *mPlayerQuque;
};

#endif /* MPRISMANAGER_H */
