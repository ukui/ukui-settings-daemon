#ifndef IXSETTINGSMANAGER_H
#define IXSETTINGSMANAGER_H


class IXsettingsManager
{
public:
    IXsettingsManager();
    virtual ~IXsettingsManager() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
};

#endif // IXSETTINGSMANAGER_H
