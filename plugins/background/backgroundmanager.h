#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H
#include <gio/gio.h>
#include <libmate-desktop/mate-bg.h>

class BackgroundManager
{
public:
    ~BackgroundManager();
    static BackgroundManager& backgroundManagerNew();
    bool backgroundManagerStart();
    void backgroundManagerStop();
private:
    BackgroundManager();

private:
    static BackgroundManager* mBackgroundManager;

    GSettings*          mSettings;
    MateBG*             mBG;
    MateBGCrossfade*    mFade;

    bool                mUsdCanDraw;
    bool                mPeonyCanDraw;
    bool                mDoFade;
    bool                mDrawInProgress;
    int                 mTimeoutID;
    GDBusProxy*         mProxy;
    int                 mProxySignalID;
};

#endif // BACKGROUNDMANAGER_H
