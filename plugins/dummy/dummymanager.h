#ifndef DUMMYMANAGER_H
#define DUMMYMANAGER_H

#include <glib-object.h>

class DummyManager
{
public:
    ~DummyManager();
    static DummyManager& DummyManagerNew();
    bool DummyManagerStart(GError **error);
    void DummyManagerStop();
private:
    DummyManager();

    static DummyManager* mDummyManager;
    bool padding;

};

#endif // DUMMYMANAGER_H
