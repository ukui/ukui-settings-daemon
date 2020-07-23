#include <QFile>
#include <QString>
#include <QList>
#include <QDir>
#include "ukui-xrdb-manager.h"
#include "clib-syslog.h"

#define midColor(x,low,high) (((x) > (high)) ? (high): (((x) < (low)) ? (low) : (x)))

ukuiXrdbManager* ukuiXrdbManager::mXrdbManager = nullptr;

ukuiXrdbManager::ukuiXrdbManager()
{
    settings = new QGSettings(SCHEMAS);
    allUsefulAdFiles = new QList<QString>();

    gtk_init(NULL,NULL);
    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_ensure_style(widget);
}

ukuiXrdbManager::~ukuiXrdbManager()
{
    if(mXrdbManager)
        delete mXrdbManager;
}

//singleton
ukuiXrdbManager* ukuiXrdbManager::ukuiXrdbManagerNew()
{
    if(nullptr == mXrdbManager)
        mXrdbManager = new ukuiXrdbManager();
    return mXrdbManager;
}

bool ukuiXrdbManager::start(GError **error)
{
    /* the initialization is done here otherwise
       ukui_settings_xsettings_load would generate
       false hit as gtk-theme-name is set to Default in
       ukui_settings_xsettings_init */
    CT_SYSLOG(LOG_DEBUG,"Starting xrdb manager!");
    if(nullptr != settings)
        connect(settings,SIGNAL(changed(const QString&)),this,SLOT(themeChanged(const QString&)));

    return true;
}

void ukuiXrdbManager::stop()
{
    CT_SYSLOG(LOG_DEBUG,"Stopping xrdb manager!");
    if(settings)
        delete settings;
    if(allUsefulAdFiles){
        allUsefulAdFiles->clear();
        delete allUsefulAdFiles;
    }

    //destroy newed GtkWidget window
    gtk_widget_destroy(widget);
}

/* func : Scan a single directory for .ad files, and return them all in a
 * QList*
 */
QList<QString>* scanAdDirectory(QString path,GError** error)
{
    QFileInfoList fileList;
    QString tmpFileName;
    QList<QString>* tmpFileList;
    QDir dir;
    int fileCount;
    int i = 0;

    dir.setPath(path);
    if(!dir.exists()){
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_EXIST,
                    "%s does not exist!",
                    path.toLatin1().data());
        return nullptr;
    }

    fileList = dir.entryInfoList();
    fileCount = fileList.count();
    tmpFileList = new QList<QString>();
    for(;i < fileCount; ++i){
        tmpFileName =  fileList.at(i).absoluteFilePath();
        if(tmpFileName.contains(".ad"))
            tmpFileList->push_back(tmpFileName);
    }

    if(tmpFileList->size() > 0)
        tmpFileList->sort();

    return tmpFileList;
}

/**
 * Scan the user and system paths, and return a list of strings in the
 * right order for processing.
 */
QList<QString>* ukuiXrdbManager::scanForFiles(GError** error)
{
    QString userHomeDir;
    QList<QString>* userAdFileList;
    QList<QString>* systemAdFileList;//remeber free
    GError* localError;

    systemAdFileList = userAdFileList = nullptr;

    //look for system ad files at /etc/xrdb/
    localError = NULL;
    systemAdFileList = scanAdDirectory(SYSTEM_AD_DIR,&localError);
    if(NULL != localError){
        g_propagate_error(error,localError);
        return nullptr;
    }

    //look for ad files at user's home.
    userHomeDir = QDir::homePath();
    if(!userHomeDir.isEmpty()){
        QString userAdDir;
        QFileInfo fileInfo;

        userAdDir = userAdDir + USER_AD_DIR;
        fileInfo.setFile(userAdDir);
        if(fileInfo.exists() && fileInfo.isDir()){
            userAdFileList = scanAdDirectory(userAdDir,&localError);
            if(NULL != localError){
                g_propagate_error(error,localError);
                systemAdFileList->clear();//memery free for QList
                return nullptr;            }
        }else
            CT_SYSLOG(LOG_INFO,"User's ad file not found at %s!",userAdDir.toLatin1().data());
    }else
        CT_SYSLOG(LOG_WARNING,"Cannot datermine user's home directory!");

    //After get all ad files,we handle it.
    if(systemAdFileList->contains(GENERAL_AD))
        systemAdFileList->removeOne(GENERAL_AD);
    if(nullptr != userAdFileList)
        removeSameItemFromFirst(systemAdFileList,userAdFileList);

    //here,we get all ad files that we needed.
    allUsefulAdFiles->append(*systemAdFileList);
    if(nullptr != userAdFileList)
        allUsefulAdFiles->append(*userAdFileList);
    allUsefulAdFiles->append(GENERAL_AD);

    //QList.append() operator is deep-copy,so we free memory.
    systemAdFileList->clear();
    delete systemAdFileList;

    if(nullptr != userAdFileList){
        userAdFileList->clear();
        delete userAdFileList;
    }

    return allUsefulAdFiles;
}

/**
 * Append the contents of a file onto the end of a QString
 */
void ukuiXrdbManager::appendFile(QString file,GError** error)
{
    GError* localError;
    QString fileContents;

    //first get all contents from file.
    localError = NULL;
    fileContents =  fileGetContents(file,&localError);

    if(NULL != localError){
        //CT_SYSLOG(LOG_ERR,"%s",localError->message);
        g_propagate_error(error,localError);
        localError = NULL;
        return;
    }

    //then append all contents to @needMerge
    if(!fileContents.isEmpty())
        needMerge.append(fileContents);
}

/* func : append contents from .Xresources or .Xdefaults  to @needMerge.
 */
void ukuiXrdbManager::appendXresourceFile(QString fileName,GError **error)
{
    QString homePath;
    QString xResources;
    QFile file;
    GError* localError;
    char* tmpName;

    homePath = QDir::homePath();
    xResources = homePath + fileName;
    file.setFileName(xResources);

    if(!file.exists()){
        tmpName = xResources.toLatin1().data();
        g_set_error(error,G_FILE_ERROR,
                    G_FILE_ERROR_NOENT,
                    "%s does not exist!",tmpName);
        //CT_SYSLOG(LOG_WARNING,"%s does not exist!",tmpName);
        return;
    }

    localError = NULL;
    appendFile(xResources,&localError);
    if(NULL != localError){
        g_propagate_error(error,localError);
        //CT_SYSLOG(LOG_WARNING,"%s",localError->message);
        localError = NULL;
    }
}

bool
write_all (int         fd,
           const char *buf,
           ulong       to_write)
{
    while (to_write > 0) {
        long count = write (fd, buf, to_write);
        if (count < 0) {
            return false;
        } else {
            to_write -= count;
            buf += count;
        }
    }

    return true;
}


void
child_watch_cb (int     pid,
                int      status,
                gpointer user_data)
{
    char *command = (char*)user_data;

    if (!WIFEXITED (status) || WEXITSTATUS (status)) {
        CT_SYSLOG (LOG_WARNING,"Command %s failed", command);
    }
}


void
spawn_with_input (const char *command,
                  const char *input)
{
    char   **argv;
    int      child_pid;
    int      inpipe;
    GError  *error;
    bool res;

    argv = NULL;
    res = g_shell_parse_argv (command, NULL, &argv, NULL);
    if (! res) {
        CT_SYSLOG (LOG_WARNING,"Unable to parse command: %s", command);
        return;
    }

    error = NULL;
    res = g_spawn_async_with_pipes (NULL,
                                    argv,
                                    NULL,
                                    (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
                                    NULL,
                                    NULL,
                                    &child_pid,
                                    &inpipe,
                                    NULL,
                                    NULL,
                                    &error);
    g_strfreev (argv);

    if (! res) {
        CT_SYSLOG(LOG_WARNING,"Could not execute %s: %s", command, error->message);
        g_error_free (error);
        return;
    }

    //sleep(15);

    if (input != NULL) {
        if (! write_all (inpipe, input, strlen (input))) {
            CT_SYSLOG(LOG_WARNING,"Could not write input to %s", command);
        }
        close (inpipe);
    }

    g_child_watch_add (child_pid, (GChildWatchFunc) child_watch_cb, (gpointer)command);

}

/* 1 according to the current theme,get color value.
 * 2 get contents from ad files.
 * 3 exec command: "xrdb -merge -quiet ....".
 */
void ukuiXrdbManager::applySettings(){
    char* command;
    GError* error;
    int i,fileNum;
    int listCount;
    char* input;

    command = "xrdb -merge -quiet";

    if(!colorDefineList.isEmpty()){
        listCount = colorDefineList.count();
        for( i = 0; i < listCount; ++i)
            needMerge.append(colorDefineList.at(i));
        colorDefineList.clear();
    }

    //first, get system ad files and user's ad files
    error = NULL;
    scanForFiles(&error);
    if(NULL != error){
        CT_SYSLOG(LOG_WARNING,"%s",error->message);
        g_error_free(error);
    }

    //second, get contents from every file,and append contends to @needMerge.
    fileNum = allUsefulAdFiles->count();
    for(i = 0; i < fileNum; ++i){
        error = NULL;
        appendFile(allUsefulAdFiles->at(i),&error);
        if(NULL != error){
            CT_SYSLOG(LOG_WARNING,"%s",error->message);
            g_error_free(error);
        }
    }

    //third, append Xresources file's contents to @needMerge.
    error = NULL;
    appendXresourceFile(USER_X_RESOURCES,&error);
    if(NULL != error){
        CT_SYSLOG(LOG_WARNING,"%s",error->message);
        g_error_free(error);
    }

    error = NULL;
    appendXresourceFile(USER_X_DEFAULTS,&error);
    if(NULL != error){
        CT_SYSLOG(LOG_WARNING,"%s",error->message);
        g_error_free(error);
    }

    //last, exec shell: @command + @needMerge
    spawn_with_input(command,needMerge.toLatin1().data());

    needMerge.clear();
    allUsefulAdFiles->clear();
}

/* func : private slots for gsettings key 'gtk-theme' changed
 * example: on ukui-3.0, @key == "ukui-black" or @key == "ukui-white"
 */
void ukuiXrdbManager::themeChanged (const QString& key)
{

    QString themeName;
    QString themeColorCssFile = nullptr;
    themeName = settings->get(key).toString();
    themeColorCssFile = SYSTEM_THEME_DIR + themeName + "/gtk-3.0/_ukui-colors.scss";

    if(!themeColorCssFile.isNull()){
        //readCssFile(themeColorCssFile);
        getColorConfigFromGtkWindow();
        applySettings();
    }
}

/*func : get color config for gtk-3.0 from css file
 */
void ukuiXrdbManager::readCssFile(const QString& cssFile){
    QFile file;
    QString lineContent;

    file.setFileName(cssFile);
    if(!file.exists()){
        CT_SYSLOG(LOG_ERR,"%s does not exist!",cssFile.toLatin1().data());
        return;
    }

    if(!file.open(QIODevice::ReadOnly)){
        CT_SYSLOG(LOG_ERR,"%s open failed! cannot get color value!",cssFile.toLatin1().data());
        return;
    }

    while(!file.atEnd()){
        lineContent = file.readLine();
        if(!lineContent.contains('#'))
            continue;
        if(lineContent.contains("button")){
            goto out;
        }

        handleLineContent(lineContent);
        lineContent.clear();
    }

out:
    file.close();
}

/* func : delete character ';' from Qstring.
 * example : "#0c0c0d;\n" -> "#0c0c0d\n"
 */
void ukuiXrdbManager::handleLineContent(const QString& lineContent){
    QStringList colorSplit;
    QString tmp1,tmp2;

    if(lineContent.isEmpty())
        return;

    colorSplit = lineContent.split(':');
    tmp1 = colorSplit.at(0);
    tmp2 = colorSplit.at(1);
    tmp2.remove(';');

    if(tmp1 == "$bg_color"){
        colorDefineList.insert(0,"#define BACKGROUND "+tmp2);
        colorDefineList.append("#define WINDOW_BACKGROUND "+tmp2);
        colorDefineList.append("#define HIGHLIGHT "+calorShade(tmp2,1.2));
    }
    else if(tmp1 == "$fg_color"){
        colorDefineList.insert(1,"#define FOREGROUND "+tmp2);
        colorDefineList.append("#define WINDOW_FOREGROUND "+tmp2);
        colorDefineList.append("#define LOWLIGHT "+calorShade(tmp2,2.0/3.0));
    }
    else if(tmp1 == "$active_bg_color")
        colorDefineList.append("#define ACTIVE_BACKGROUND "+tmp2);
    else if(tmp1 == "$active_fg_color")
        colorDefineList.append("#define ACTIVE_FOREGROUND "+tmp2);
    else if(tmp1 == "$selected_bg_color")
        colorDefineList.append("#define SELECT_BACKGROUND "+tmp2);
    else if(tmp1 == "$selected_fg_color")
        colorDefineList.append("#define SELECT_FOREGROUND "+tmp2);
    else if(tmp1 == "$disable_bg_color")
        colorDefineList.append("#define INACTIVE_BACKGROUND "+tmp2);
    else if(tmp1 == "$disable_fg_color")
        colorDefineList.append("#define INACTIVE_FOREGROUND "+tmp2);
}

QString ukuiXrdbManager::calorShade(const QString color,double shade){
    int red,green,blue;
    QString tmp;

    red = color.mid(1,2).toInt(nullptr,16);
    green = color.mid(3,2).toInt(nullptr,16);
    blue = color.mid(5,2).toInt(nullptr,16);

    red = midColor(red*shade,0,255);
    green = midColor(green*shade,0,255);
    blue = midColor(green*shade,0,255);

    tmp = QString("#%1%2%3\n").arg(red,2,16,QLatin1Char('0'))
            .arg(green,2,16,QLatin1Char('0')).arg(blue,2,16,QLatin1Char('0'));

    return tmp;
}

/* func : remove one item from first,if second have this item too.
 *        the point is : name is same.
 * example: first.at(3) == "/etc/xrdb/tmp1.ad"
 *          second.at(1) == $QDir::homePath + "/.config/ukui/tmp1.ad"
 *          then exec : first.removeAt(3);
 */
void ukuiXrdbManager::removeSameItemFromFirst(QList<QString>* first,
                             QList<QString>* second){
    QFileInfo tmpFirstName;
    QFileInfo tmpSecondName;
    QString firstBaseName;
    QString secondBaseName;
    int i,j;
    int firstSize,secondSize;
//    if(first->isEmpty() || second->isEmpty()){
//        return;
//    }

    first->length();
    firstSize = first->size();
    secondSize = second->size();

    for(i = 0; i < firstSize; ++i){
        firstBaseName.clear();
        tmpFirstName.setFile(first->at(i));
        firstBaseName = tmpFirstName.fileName();
        for(j = 0; j < secondSize; ++j){
            secondBaseName.clear();
            tmpSecondName.setFile(second->at(j));
            secondBaseName = tmpSecondName.fileName();
            if(firstBaseName == secondBaseName){
                first->removeAt(i);
                firstSize -= 1;
                break;
            }
        }
    }
}

/* func : get all contents from file.
 */
QString ukuiXrdbManager::fileGetContents(QString fileName,GError **error)
{
    QFile file;
    QString fileContents;

    file.setFileName(fileName);
    if(!file.exists()){
        g_set_error(error,G_FILE_ERROR,
                    G_FILE_ERROR_NOENT,
                    "%s does not exists!",fileName.toLatin1().data());
        return nullptr;
    }

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        g_set_error(error,G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "%s open failed!",fileName.toLatin1().data());
        return nullptr;
    }

    fileContents = file.readAll();

    return fileContents;
}

void ukuiXrdbManager::getColorConfigFromGtkWindow()
{
    GtkWidget* widget;
    GtkStyle* style;

    style = gtk_widget_get_style(widget);

    appendColor("BACKGROUND",&style->bg[GTK_STATE_NORMAL]);
    appendColor("FOREGROUND",&style->fg[GTK_STATE_NORMAL]);
    appendColor("SELECT_BACKGROUND",&style->bg[GTK_STATE_SELECTED]);
    appendColor("SELECT_FOREGROUND",&style->text[GTK_STATE_SELECTED]);
    appendColor("WINDOW_BACKGROUND",&style->base[GTK_STATE_NORMAL]);
    appendColor("WINDOW_FOREGROUND",&style->text[GTK_STATE_NORMAL]);
    appendColor("INACTIVE_BACKGROUND",&style->bg[GTK_STATE_INSENSITIVE]);
    appendColor("INACTIVE_FOREGROUND",&style->text[GTK_STATE_INSENSITIVE]);
    appendColor("ACTIVE_BACKGROUND",&style->bg[GTK_STATE_SELECTED]);
    appendColor("ACTIVE_FOREGROUND",&style->text[GTK_STATE_SELECTED]);

    colorShade2("HIGHLIGHT",&style->bg[GTK_STATE_NORMAL],1.2);
    colorShade2("LOWLIGHT",&style->fg[GTK_STATE_NORMAL],2.0/3.0);
}

/* func : Gets the hexadecimal value of the color
 * example : #define BACKGROUND #ffffff\n
 */
void ukuiXrdbManager::appendColor(QString name,GdkColor* color)
{
    QString tmp;
    tmp = QString("#%1%2%3\n").arg(color->red>>8,2,16,QLatin1Char('0'))
                .arg(color->green>>8,2,16,QLatin1Char('0')).arg(color->blue>>8,2,16,QLatin1Char('0'));
    colorDefineList.append("#define " + name + " " + tmp);
}

void ukuiXrdbManager::colorShade2(QString name,GdkColor* color,double shade)
{
    GdkColor tmp;

    tmp.red = midColor((color->red)*shade,0,0xFFFF);
    tmp.green = midColor((color->green)*shade,0,0xFFFF);
    tmp.blue = midColor((color->blue)*shade,0,0xFFFF);

    appendColor(name,&tmp);
}
