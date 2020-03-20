#ifndef MEDIAKEYMANAGER_H
#define MEDIAKEYMANAGER_H

#include <QObject>

class MediakeyManager : public QObject
{
    Q_OBJECT
public:
    explicit MediakeyManager(QObject *parent = nullptr);

    bool mediakeyStart ();
    bool mediakeyStop ();

    // bool mediakeyGrab ();
    // bool mediakeyRelease ();

Q_SIGNALS:

};

#endif // MEDIAKEYMANAGER_H
