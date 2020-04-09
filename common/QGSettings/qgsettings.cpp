#include "qgsettings.h"
#include "qconftype.h"

#include <glib.h>
#include <gio/gio.h>

#include <QDebug>
#include <QString>

struct QGSettingsPrivate
{
    QByteArray          path;
    GSettingsSchema     *schema;
    QByteArray          schemaId;
    GSettings           *settings;
    gulong              signalHandlerId;

    static void settingChanged(GSettings *settings, const gchar *key, gpointer userData);
};

void QGSettingsPrivate::settingChanged(GSettings *, const gchar *key, gpointer userData)
{
    QGSettings *self = (QGSettings *)userData;

    /**
     * 这里不属于 QObject的子类，只能通过此方法强制调用 QObject 子类的方法或信号
     *
     * Qt::QueuedConnection         发送一个QEvent，并在应用程序进入主事件循环后立即调用该成员。
     * Qt::DirectConnection         立即调用
     * Qt::BlockingQueuedConnection 则将以与Qt::QueuedConnection相同的方式调用该方法，除了当前线程将阻塞直到事件被传递。使用此连接类型在同一线程中的对象之间进行通信将导致死锁
     * Qt::AutoConnection           则如果obj与调用者位于同一个线程中，则会同步调用该成员; 否则它将异步调用该成员
     *
     */
    QMetaObject::invokeMethod(self, "changed", Qt::AutoConnection, Q_ARG(QString, key));
}


QGSettings::QGSettings(const QByteArray &schemaId, const QByteArray &path, QObject *parent) : QObject(parent)
{
    mPriv = new QGSettingsPrivate;
    mPriv->schemaId = schemaId;
    mPriv->path = path;

    if (mPriv->path.isEmpty()) {
        mPriv->settings = g_settings_new(mPriv->schemaId.constData());
    } else {
        mPriv->settings = g_settings_new_with_path(mPriv->schemaId.constData(), mPriv->path.constData());
    }
    g_object_get(mPriv->settings, "settings-schema", &mPriv->schema, NULL);
    mPriv->signalHandlerId = g_signal_connect(mPriv->settings, "changed", G_CALLBACK(QGSettingsPrivate::settingChanged), this);
}

QGSettings::~QGSettings()
{
    if (mPriv->schema) {
        g_settings_sync ();
        g_signal_handler_disconnect(mPriv->settings, mPriv->signalHandlerId);
        g_object_unref (mPriv->settings);
        g_settings_schema_unref (mPriv->schema);
    }
    delete mPriv;
}

QVariant QGSettings::get(const QString &key) const
{
    gchar *gkey = unqtify_name(key);
    GVariant *value = g_settings_get_value(mPriv->settings, gkey);
    QVariant qvalue = qconf_types_to_qvariant(value);
    g_variant_unref(value);
    g_free(gkey);

    return qvalue;
}

void QGSettings::set(const QString &key, const QVariant &value)
{
    if (!trySet(key, value))
        qWarning("unable to set key '%s' to value '%s'", key.toUtf8().constData(), value.toString().toUtf8().constData());
}

bool QGSettings::trySet(const QString &key, const QVariant &value)
{
    gchar *gkey = unqtify_name(key);
    bool success = false;

    /* fetch current value to find out the exact type */
    GVariant *cur = g_settings_get_value(mPriv->settings, gkey);

    GVariant *new_value = qconf_types_collect_from_variant(g_variant_get_type (cur), value);
    if (new_value)
        success = g_settings_set_value(mPriv->settings, gkey, new_value);

    g_free(gkey);
    g_variant_unref (cur);

    return success;
}

void QGSettings::delay()
{
    g_settings_delay(mPriv->settings);
}
void QGSettings::apply()
{
    g_settings_apply(mPriv->settings);
}

QStringList QGSettings::keys() const
{
    QStringList list;
    gchar **keys = g_settings_list_keys(mPriv->settings);
    for (int i = 0; keys[i]; i++)
        list.append(keys[i]);
//    list.append(qtify_name(keys[i]));

    g_strfreev(keys);

    return list;
}

QVariantList QGSettings::choices(const QString &qkey) const
{
    gchar *key = unqtify_name (qkey);
    GSettingsSchemaKey *schema_key = g_settings_schema_get_key (mPriv->schema, key);
    GVariant *range = g_settings_schema_key_get_range(schema_key);
    g_settings_schema_key_unref (schema_key);
    g_free(key);

    if (range == NULL)
        return QVariantList();

    const gchar *type;
    GVariant *value;
    g_variant_get(range, "(&sv)", &type, &value);

    QVariantList choices;
    if (g_str_equal(type, "enum")) {
        GVariantIter iter;
        GVariant *child;

        g_variant_iter_init (&iter, value);
        while ((child = g_variant_iter_next_value(&iter))) {
            choices.append(qconf_types_to_qvariant(child));
            g_variant_unref(child);
        }
    }

    g_variant_unref (value);
    g_variant_unref (range);

    return choices;
}

void QGSettings::reset(const QString &qkey)
{
    gchar *key = unqtify_name(qkey);
    g_settings_reset(mPriv->settings, key);
    g_free(key);
}

bool QGSettings::isSchemaInstalled(const QByteArray &schemaId)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
    GSettingsSchema *schema = g_settings_schema_source_lookup (source, schemaId.constData(), TRUE);
    if (schema) {
        g_settings_schema_unref (schema);
        return true;
    } else {
        return false;
    }
}
