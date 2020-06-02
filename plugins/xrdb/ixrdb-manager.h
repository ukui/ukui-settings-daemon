#ifndef IXRDBMANAGER_H
#define IXRDBMANAGER_H

#include <glib.h>

class IXrdbManager {
public:
    IXrdbManager(){};
    virtual ~IXrdbManager() = 0;
    virtual bool start(GError**) = 0;
    virtual void stop() = 0;
};

#endif // IXRDBMANAGER_H
