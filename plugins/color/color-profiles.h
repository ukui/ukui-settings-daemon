#ifndef COLORPROFILES_H
#define COLORPROFILES_H

#include <QObject>
#include "config.h"
#include <stdio.h>
#include <glib/gi18n.h>
#include <colord.h>

class ColorProfiles : public QObject
{
    Q_OBJECT

public:
    ColorProfiles();
    ColorProfiles(ColorProfiles&)=delete;
    ~ColorProfiles();

    bool ColorProfilesStart();
    void ColorProfilesStop();

public:
    static void SessionIccStoreAddedCb (CdIccStore *icc_store,
                                        CdIcc *icc,
                                        ColorProfiles *profiles);
    static void SessionIccStoreRemovedCb (CdIccStore *icc_store,
                                          CdIcc *icc,
                                          ColorProfiles *profiles);
    static void SessionFindProfileByFilenameCb (GObject *object,
                                                GAsyncResult *res,
                                                gpointer user_data);
    static void SessionClientConnectCb (GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data);

private:
    GCancellable    *cancellable;
    CdClient        *client;
    CdIccStore      *icc_store;

};

#endif // COLORPROFILES_H
