#ifndef UKUIXRDBMANAGER_H
#define UKUIXRDBMANAGER_H

#include <QObject>
#include <QGSettings>
#include "ixrdb-manager.h"

/* qt会将glib的signals成员识别为宏，所以取消该宏
 * 如果程序中需要使用qt的信号机制，则使用Q_SIGNALS即可
 */
#ifdef signals
#undef signals
#endif

extern "C"{
#include <unistd.h>
#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
}

#define SYSCONFDIR       "/etc"
#define SYSTEM_AD_DIR    SYSCONFDIR "/xrdb"
#define GENERAL_AD       SYSTEM_AD_DIR "/General.ad"
#define USER_AD_DIR      ".config/ukui/xrdb"
#define USER_X_RESOURCES ".Xresources"
#define USER_X_DEFAULTS  ".Xdefaults"

#define SCHEMAS          "org.mate.interface"
#define SYSTEM_THEME_DIR "/usr/share/themes/"

class ukuiXrdbManager:public QObject,public IXrdbManager
{
    Q_OBJECT
public:
    ~ukuiXrdbManager();
    static ukuiXrdbManager* ukuiXrdbManagerNew();
    bool start(GError **error);
    void stop();

private:
    ukuiXrdbManager();
    void readCssFile(const QString& cssFile);
    void handleLineContent(const QString& lineContent);
    QString calorShade(const QString color,double shade);
    QList<QString>* scanForFiles(GError** error);
    void removeSameItemFromFirst(QList<QString>* first,
                                 QList<QString>* second);
    void applySettings();
    void appendFile(QString file,GError** error);
    void appendXresourceFile(QString fileName,GError **error);

    QString fileGetContents(QString fileName,GError **error);
    void getColorConfigFromGtkWindow();
    void appendColor(QString name,GdkColor* color);
    void colorShade2(QString name,GdkColor* color,double shade);
private Q_SLOTS:
    void themeChanged (const QString& key);

private:
    static ukuiXrdbManager* mXrdbManager;
    QGSettings* settings;
    GtkWidget* widget;

    //after call spawn_with_input(),the following three members must be Empty.
    QList<QString>* allUsefulAdFiles;
    QStringList colorDefineList;
    QString needMerge;
};

#endif // UKUIXRDBMANAGER_H
