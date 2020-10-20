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
#ifndef BACKGROUND_MANAGER_H
#define BACKGROUND_MANAGER_H
#include <QWidget>
#include <QObject>
#include <QVariantAnimation>
#include <QGSettings/QGSettings>
#include <QPixmap>
#include <QRect>
#include <QPainter>
#include <QWindow>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-bg.h>

class BackgroundManager : public QWidget
{
    Q_OBJECT
public:
    BackgroundManager(QScreen *screen, bool is_primary,QWidget *parent = nullptr);
    static BackgroundManager* getInstance();
    ~BackgroundManager();

public:
    void remove_background ();
    void paintEvent(QPaintEvent *e);

    void setIsPrimary(bool is_primary);
    QScreen *getScreen() {
        return m_screen;
    }
    void setScreen(QScreen *screen);
private:
    QList<BackgroundManager*> m_window_list;
protected:
    void initGSettings();
private:
    void scaleBg(const QRect &geometry);
    void geometryChangedProcess(const QRect &geometry);

protected Q_SLOTS:
    bool isPrimaryScreen(QScreen *screen);
public Q_SLOTS:
    void updateWinGeometry();
    void updateView();
    void checkWindowProcess();
    void setup_Background(const QString &key);

    void screenAddedProcess(QScreen *screen);
    void addWindow(QScreen *screen, bool checkPrimay = true);
Q_SIGNALS:
    void checkWindow();
private:
    BackgroundManager()=delete;
    BackgroundManager(BackgroundManager&) = delete;
    BackgroundManager& operator= (const BackgroundManager&) = delete;

private:
    QGSettings *bSettingOld;
    QGSettings *bSettingNew;

    QString pFilename;
    QString qFilename;

    QPixmap m_bg_font_pixmap;
    QPixmap m_bg_back_pixmap;
    QPixmap m_bg_back_cache_pixmap;
    QPixmap m_bg_font_cache_pixmap;
    QColor m_last_pure_color = Qt::transparent;
    QColor m_color_to_be_set = Qt::transparent;

    bool settingsOldCreate;
    bool settingsNewCreate;
    bool m_is_primary;
    bool canDraw;
    QScreen *m_screen;

    QVariantAnimation *m_opacity = nullptr;
    static BackgroundManager*   mBackgroundManager;
};

#endif // BACKGROUND_MANAGER_H
