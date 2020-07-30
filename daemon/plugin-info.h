/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PluginInfo_H
#define PluginInfo_H
#include "plugin-interface.h"

#include <glib-object.h>
#include <QLibrary>
#include <QObject>
#include <string>

#include <QGSettings/qgsettings.h>

namespace UkuiSettingsDaemon {
class PluginInfo;
}

class PluginInfo : public QObject
{
    Q_OBJECT
public:
    explicit PluginInfo()=delete;
    PluginInfo(QString& fileName);
    ~PluginInfo();

    bool pluginEnabled ();
    bool pluginActivate ();
    bool pluginDeactivate ();
    bool pluginIsactivate ();
    bool pluginIsAvailable ();

    int getPluginPriority ();
    QString& getPluginName ();
    QString& getPluginWebsite ();
    QString& getPluginLocation ();
    QString& getPluginCopyright ();
    QString& getPluginDescription ();
    QList<QString>& getPluginAuthors ();

    void setPluginPriority (int priority);
    void setPluginSchema (QString& schema);

    bool operator== (PluginInfo&);

public Q_SLOTS:
    void pluginSchemaSlot (QString key);

private:
    friend bool loadPluginModule(PluginInfo&);

private:
    int                     mPriority;

    bool                    mActive;
    bool                    mEnabled;
    bool                    mAvailable;

    QString                 mFile;
    QString                 mName;
    QString                 mDesc;
    QString                 mWebsite;
    QString                 mLocation;
    QString                 mCopyright;
    QGSettings*             mSettings;

    QLibrary*               mModule;
    PluginInterface*        mPlugin;

    QList<QString>*         mAuthors;
};

#endif // PluginInfo_H
