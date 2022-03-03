#ifndef MATEOUPUT_H
#define MATEOUPUT_H
#include <QObject>
#include <QList>
#include "clib-syslog.h"

#define DEFINE_QSTRING(var) QString var; QString get##var(){return var;} void set##var(QString e){var = e;}
//#define DEFINE_QSTRING(var) QString get##var(){return var;} void set##var(QString e){e = var;}

class XrandrMateOuput : public QObject{
Q_OBJECT
    Q_PROPERTY(QString name READ getname WRITE setname)
    Q_PROPERTY(QString vendor READ getvendor WRITE setvendor)
    Q_PROPERTY(QString product READ getproduct WRITE setproduct)
    Q_PROPERTY(QString serial READ getserial WRITE setserial)
    Q_PROPERTY(QString width READ getwidth WRITE setwidth)
    Q_PROPERTY(QString height READ getheight WRITE setheight)
    Q_PROPERTY(QString rate READ getrate WRITE setrate)
    Q_PROPERTY(QString x READ getx WRITE setx)
    Q_PROPERTY(QString y READ gety WRITE sety)
    Q_PROPERTY(QString rotation READ getrotation WRITE setrotation)
    Q_PROPERTY(QString primary READ getprimary WRITE setprimary)

private:

    DEFINE_QSTRING(name)
    DEFINE_QSTRING(vendor)
    DEFINE_QSTRING(product)
    DEFINE_QSTRING(serial)
    DEFINE_QSTRING(width)
    DEFINE_QSTRING(height)
    DEFINE_QSTRING(rate)
    DEFINE_QSTRING(x)
    DEFINE_QSTRING(y)
    DEFINE_QSTRING(rotation)
    DEFINE_QSTRING(primary)
};

class MateConfig{
public:
    QString m_clone;
    QList<XrandrMateOuput> m_outputList;
};

#endif // MATEOUPUT_H
