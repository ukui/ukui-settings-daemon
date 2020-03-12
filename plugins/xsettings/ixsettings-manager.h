#ifndef IXSETTINGSMANAGER_H
#define IXSETTINGSMANAGER_H

#include <glib.h>
class IXsettingsManager
{
public:
    IXsettingsManager();
    virtual ~IXsettingsManager() = 0;
    virtual int start(GError **error) = 0;
    virtual int stop() = 0;
};

#endif // IXSETTINGSMANAGER_H
