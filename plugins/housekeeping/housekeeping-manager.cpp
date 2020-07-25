#include "config.h"

#include "housekeeping-manager.h"
#include "clib-syslog.h"

#include "qtimer.h"

/* General */
#define INTERVAL_ONCE_A_DAY     24*60*60
//#define INTERVAL_TWO_MINUTES    2*60
#define INTERVAL_TWO_MINUTES    2
/* Thumbnail cleaner */
#define THUMB_CACHE_SCHEMA      "org.mate.thumbnail-cache"
#define THUMB_CACHE_KEY_AGE     "maximum-age"
#define THUMB_CACHE_KEY_SIZE	"maximum-size"

HousekeepingManager  *HousekeepingManager::mHouseManager = nullptr;
DIskSpace   *HousekeepingManager::mDisk = nullptr;

typedef struct {
    long now;
    long max_age;
    signed long total_size;
    signed long max_size;
} PurgeData;


typedef struct {
    time_t  mtime;
    char   *path;
    long    size;
} ThumbData;

/*
 * 初始化DIskSpace， QGSettings，两个QTimer
*/
HousekeepingManager::HousekeepingManager()
{
    mDisk = new DIskSpace();
    settings = new QGSettings(THUMB_CACHE_SCHEMA);
    long_term_handler = new QTimer(this);
    connect(long_term_handler, SIGNAL(timeout()), this, SLOT(do_cleanup()));
    short_term_handler = new QTimer(this);
    connect(short_term_handler, SIGNAL(timeout()), this, SLOT(do_cleanup_once()));
}

/*
 * 清理DIskSpace， QGSettings，两个QTimer
*/
HousekeepingManager::~HousekeepingManager()
{
    delete mDisk;
    delete settings;
    delete long_term_handler;
    delete short_term_handler;
}



static GList *
read_dir_for_purge (const char *path, GList *files)
{
    GFile           *read_path;
    GFileEnumerator *enum_dir;

    read_path = g_file_new_for_path (path);
    enum_dir = g_file_enumerate_children (read_path,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                                          G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL,
                                          NULL);

    if (enum_dir != NULL) {
        GFileInfo *info;
        while ((info = g_file_enumerator_next_file (enum_dir, NULL, NULL)) != NULL)
        {

            const char *name;
            name = g_file_info_get_name (info);

            if (strlen (name) == 36 && strcmp (name + 32, ".png") == 0) {
                ThumbData *td;
                GFile     *entry;
                char      *entry_path;
                GTimeVal   mod_time;

                entry = g_file_get_child (read_path, name);
                entry_path = g_file_get_path (entry);
                g_object_unref (entry);

                g_file_info_get_modification_time (info, &mod_time);

                td = g_new0 (ThumbData, 1);
                td->path = entry_path;
                td->mtime = mod_time.tv_sec;
                td->size = g_file_info_get_size (info);

                files = g_list_prepend (files, td);
            }
            g_object_unref (info);
        }
        g_object_unref (enum_dir);
    }
    g_object_unref (read_path);

    return files;
}

static void
purge_old_thumbnails (ThumbData *info, PurgeData *purge_data)
{
    if ((purge_data->now - info->mtime) > purge_data->max_age) {
        g_unlink (info->path);
        info->size = 0;
    } else {
        purge_data->total_size += info->size;
    }
}

static int
sort_file_mtime (ThumbData *file1, ThumbData *file2)
{
    return file1->mtime - file2->mtime;
}

static void
thumb_data_free (gpointer data)
{
    ThumbData *info = (ThumbData *)data;

    if (info) {
        g_free (info->path);
        g_free (info);
    }
}

void HousekeepingManager::purge_thumbnail_cache ()
{
    char      *path;
    GList     *files;
    PurgeData  purge_data;
    GTimeVal   current_time;


    purge_data.max_age  = settings->get(THUMB_CACHE_KEY_AGE).toInt() * 24 * 60 * 60;
    purge_data.max_size = settings->get(THUMB_CACHE_KEY_SIZE).toInt() * 1024 * 1024;

    /* if both are set to -1, we don't need to read anything */
    if ((purge_data.max_age < 0) && (purge_data.max_size < 0))
        return;

    path = g_build_filename (g_get_user_cache_dir (),
                             "thumbnails",
                             "normal",
                             NULL);
    files = read_dir_for_purge (path, NULL);
    g_free (path);

    path = g_build_filename (g_get_user_cache_dir (),
                             "thumbnails",
                             "large",
                             NULL);
    files = read_dir_for_purge (path, files);
    g_free (path);

    path = g_build_filename (g_get_user_cache_dir (),
                             "thumbnails",
                             "fail",
                             "ukui-thumbnail-factory",
                             NULL);
    files = read_dir_for_purge (path, files);
    g_free (path);

    g_get_current_time (&current_time);

    purge_data.now = current_time.tv_sec;
    purge_data.total_size = 0;

    if (purge_data.max_age >= 0)
        g_list_foreach (files, (GFunc) purge_old_thumbnails, &purge_data);

    if ((purge_data.total_size > purge_data.max_size) && (purge_data.max_size >= 0)) {
        GList *scan;
        files = g_list_sort (files, (GCompareFunc) sort_file_mtime);
        for (scan = files; scan && (purge_data.total_size > purge_data.max_size); scan = scan->next) {
            ThumbData *info = (ThumbData *)scan->data;
            g_unlink (info->path);
            purge_data.total_size -= info->size;
        }
    }

    g_list_foreach (files, (GFunc) thumb_data_free, NULL);
    g_list_free (files);
}


bool HousekeepingManager::do_cleanup ()
{
    purge_thumbnail_cache ();
    return true;
}

bool HousekeepingManager::do_cleanup_once ()
{
    do_cleanup ();
    short_term_handler->stop();
    return false;
}


void HousekeepingManager::do_cleanup_soon()
{
    short_term_handler->start(INTERVAL_TWO_MINUTES);
}

void HousekeepingManager::settings_changed_callback(QString key)
{
    do_cleanup_soon ();
}

bool HousekeepingManager::HousekeepingManagerStart()
{
    syslog(LOG_ERR,"Housekeeping Manager Start ");

    mDisk->UsdLdsmSetup(false);

    connect (settings, SIGNAL(changed(QString("changeds"))),this, SLOT(HousekeepingManager::settings_changed_callback(QString)));

    /* Clean once, a few minutes after start-up */
    do_cleanup_soon ();

    long_term_handler->start(INTERVAL_ONCE_A_DAY);

    return true;
}

void HousekeepingManager::HousekeepingManagerStop()
{
    syslog(LOG_DEBUG, "Housekeeping Manager Stop");
    // 时间
    if (short_term_handler) {
        short_term_handler->stop();
    }
    // 时间
    if (long_term_handler) {
        long_term_handler->stop();

        /* Do a clean-up on shutdown if and only if the size or age
         * limits have been set to a paranoid level of cleaning (zero)
         */
        if ((settings->get(THUMB_CACHE_KEY_AGE).toInt() == 0)  ||
                (settings->get(THUMB_CACHE_KEY_SIZE).toInt() == 0)) {
            do_cleanup ();
        }
    }
    mDisk->UsdLdsmClean();
}
