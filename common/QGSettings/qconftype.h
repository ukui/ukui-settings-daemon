#ifndef QCONFTYPE_H
#define QCONFTYPE_H

#include <glib.h>
#include <QVariant>

QVariant::Type qconf_types_convert (const GVariantType* gtype);
GVariant* qconf_types_collect (const GVariantType* gtype, const void* argument);
GVariant* qconf_types_collect_from_variant(const GVariantType* gtype, const QVariant& v);
QVariant qconf_types_to_qvariant (GVariant* value);
void qconf_types_unpack (GVariant* value, void* argument);

QString qtify_name(const char *name);
gchar * unqtify_name(const QString &name);

#endif // QCONFTYPE_H
