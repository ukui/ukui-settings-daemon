#ifndef IXRDBMANAGER_H
#define IXRDBMANAGER_H

class IXrdbManager {
public:
    IXrdbManager(){};
    virtual ~IXrdbManager() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
};

#endif // IXRDBMANAGER_H
