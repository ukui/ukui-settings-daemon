#ifndef USDBASECLASS_H
#define USDBASECLASS_H
#include <QObject>
#include <QMetaEnum>
#include <ukuisdk/kylin-com4cxx.h>

class UsdBaseClass: public QObject
{
    Q_OBJECT

public:
    UsdBaseClass();
    ~UsdBaseClass();
    enum eScreenMode {
        firstScreenMode = 0,
        cloneScreenMode,
        extendScreenMode,
        secondScreenMode};

    Q_ENUM(eScreenMode)

    static bool isTablet();

    static bool isMasterSP1();

    static bool is9X0();

    static bool isWayland();

    static bool isXcb();

    static bool isNotebook();

    static bool isLoongarch();

    static bool isUseXEventAsShutKey();
};

#endif // USDBASECLASS_H
