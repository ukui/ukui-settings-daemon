#include "qconftype.h"

#include <QStringList>

/**
 *  GVariant Type Name/Code      C++ Type Name          QVariant Type Name
 *  --------------------------------------------------------------------------
 *  boolean            b         bool                   QVariant::Bool
 *  byte               y         char                   QVariant::Char
 *  int16              n         int                    QVariant::Int
 *  uint16             q         unsigned int           QVariant::UInt
 *  int32              i         int                    QVariant::Int
 *  uint32             u         unsigned int           QVariant::UInt
 *  int64              x         long long              QVariant::LongLong
 *  uint64             t         unsigned long long     QVariant::ULongLong
 *  double             d         double                 QVariant::Double
 *  string             s         QString                QVariant::String
 *  string array*      as        QStringList            QVariant::StringList
 *  byte array         ay        QByteArray             QVariant::ByteArray
 *  dictionary         a{ss}     QVariantMap            QVariant::Map
 */

QVariant::Type qconf_types_convert(const GVariantType *gtype)
{
    switch (g_variant_type_peek_string(gtype)[0]) {
        case G_VARIANT_CLASS_BOOLEAN:
            return QVariant::Bool;

        case G_VARIANT_CLASS_BYTE:
            return QVariant::Char;

        case G_VARIANT_CLASS_INT16:
            return QVariant::Int;

        case G_VARIANT_CLASS_UINT16:
            return QVariant::UInt;

        case G_VARIANT_CLASS_INT32:
            return QVariant::Int;

        case G_VARIANT_CLASS_UINT32:
            return QVariant::UInt;

        case G_VARIANT_CLASS_INT64:
            return QVariant::LongLong;

        case G_VARIANT_CLASS_UINT64:
            return QVariant::ULongLong;

        case G_VARIANT_CLASS_DOUBLE:
            return QVariant::Double;

        case G_VARIANT_CLASS_STRING:
            return QVariant::String;

        case G_VARIANT_CLASS_ARRAY:
            if (g_variant_type_equal(gtype, G_VARIANT_TYPE_STRING_ARRAY))
                return QVariant::StringList;

            else if (g_variant_type_equal(gtype, G_VARIANT_TYPE_BYTESTRING))
                return QVariant::ByteArray;

            else if (g_variant_type_equal(gtype, G_VARIANT_TYPE ("a{ss}")))
                return QVariant::Map;

            // fall through
        default:
            return QVariant::Invalid;
        }
}

GVariant *qconf_types_collect_from_variant(const GVariantType *gtype, const QVariant &v)
{
    switch (g_variant_type_peek_string(gtype)[0]) {
        case G_VARIANT_CLASS_BOOLEAN:
            return g_variant_new_boolean(v.toBool());

        case G_VARIANT_CLASS_BYTE:
            return g_variant_new_byte(v.toChar().cell());

        case G_VARIANT_CLASS_INT16:
            return g_variant_new_int16(v.toInt());

        case G_VARIANT_CLASS_UINT16:
            return g_variant_new_uint16(v.toUInt());

        case G_VARIANT_CLASS_INT32:
            return g_variant_new_int32(v.toInt());

        case G_VARIANT_CLASS_UINT32:
            return g_variant_new_uint32(v.toUInt());

        case G_VARIANT_CLASS_INT64:
            return g_variant_new_int64(v.toLongLong());

        case G_VARIANT_CLASS_UINT64:
            return g_variant_new_int64(v.toULongLong());

        case G_VARIANT_CLASS_DOUBLE:
            return g_variant_new_double(v.toDouble());

        case G_VARIANT_CLASS_STRING:
            return g_variant_new_string(v.toString().toUtf8());

        case G_VARIANT_CLASS_ARRAY:
            if (g_variant_type_equal(gtype, G_VARIANT_TYPE_STRING_ARRAY)) {
                const QStringList list = v.toStringList();
                GVariantBuilder builder;

                g_variant_builder_init(&builder, G_VARIANT_TYPE_STRING_ARRAY);

                Q_FOREACH (const QString& string, list)
                  g_variant_builder_add(&builder, "s", string.toUtf8().constData());

                return g_variant_builder_end(&builder);
            } else if (g_variant_type_equal(gtype, G_VARIANT_TYPE_BYTESTRING)) {
                const QByteArray array = v.toByteArray();
                gsize size = array.size();
                gpointer data;

                data = g_memdup(array.data(), size);

                return g_variant_new_from_data(G_VARIANT_TYPE_BYTESTRING,
                                               data, size, TRUE, g_free, data);
            } else if (g_variant_type_equal(gtype, G_VARIANT_TYPE("a{ss}"))) {
                GVariantBuilder builder;
                g_variant_builder_init(&builder, G_VARIANT_TYPE ("a{ss}"));
                QMapIterator<QString, QVariant> it(v.toMap());
                while (it.hasNext()) {
                    it.next();
                    QByteArray key = it.key().toUtf8();
                    QByteArray val = it.value().toByteArray();
                    g_variant_builder_add (&builder, "{ss}", key.constData(), val.constData());
                }
                return g_variant_builder_end (&builder);
            }

            // fall through
        default:
            return NULL;
    }
}

QVariant qconf_types_to_qvariant(GVariant *value)
{
    switch (g_variant_classify(value)) {
        case G_VARIANT_CLASS_BOOLEAN:
            return QVariant((bool) g_variant_get_boolean(value));

        case G_VARIANT_CLASS_BYTE:
            return QVariant((char) g_variant_get_byte(value));

        case G_VARIANT_CLASS_INT16:
            return QVariant((int) g_variant_get_int16(value));

        case G_VARIANT_CLASS_UINT16:
            return QVariant((unsigned int) g_variant_get_uint16(value));

        case G_VARIANT_CLASS_INT32:
            return QVariant((int) g_variant_get_int32(value));

        case G_VARIANT_CLASS_UINT32:
            return QVariant((unsigned int) g_variant_get_uint32(value));

        case G_VARIANT_CLASS_INT64:
            return QVariant((long long) g_variant_get_int64(value));

        case G_VARIANT_CLASS_UINT64:
            return QVariant((unsigned long long) g_variant_get_uint64(value));

        case G_VARIANT_CLASS_DOUBLE:
            return QVariant(g_variant_get_double(value));

        case G_VARIANT_CLASS_STRING:
            return QVariant(g_variant_get_string(value, NULL));

        case G_VARIANT_CLASS_ARRAY:
            if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
                GVariantIter iter;
                QStringList list;
                const gchar *str;

                g_variant_iter_init (&iter, value);
                while (g_variant_iter_next (&iter, "&s", &str))
                    list.append (str);

                return QVariant(list);
            } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_BYTESTRING)) {
                return QVariant(QByteArray(g_variant_get_bytestring(value)));
            } else if (g_variant_is_of_type(value, G_VARIANT_TYPE("a{ss}"))) {
                GVariantIter iter;
                QMap<QString, QVariant> map;
                const gchar *key;
                const gchar *val;

                g_variant_iter_init (&iter, value);
                while (g_variant_iter_next (&iter, "{&s&s}", &key, &val))
                    map.insert(key, QVariant(val));

                return map;
            }

            // fall through
        default:
            g_assert_not_reached();
        }
}

// trans 'aa-bb' to 'aaBb'
QString qtify_name(const char *name)
{
    bool next_cap = false;
    QString result;

    while (*name) {
        if (*name == '-') {
            next_cap = true;
        } else if (next_cap) {
            result.append(QChar(*name).toUpper().toLatin1());
            next_cap = false;
        } else {
            result.append(*name);
        }

        name++;
    }

    return result;
}

gchar * unqtify_name(const QString &name)
{
    const gchar *p;
    QByteArray bytes;
    GString *str;

    bytes = name.toUtf8();
    str = g_string_new (NULL);

    for (p = bytes.constData(); *p; p++) {
        const QChar c(*p);
        if (c.isUpper()) {
            g_string_append_c (str, '-');
            g_string_append_c (str, c.toLower().toLatin1());
        }
        else {
            g_string_append_c (str, *p);
        }
    }

    return g_string_free(str, FALSE);
}
