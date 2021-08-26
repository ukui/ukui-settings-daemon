#ifndef USDBASECLASS_H
#define USDBASECLASS_H


class UsdBaseClass
{
public:
    UsdBaseClass();
    ~UsdBaseClass();

static bool isTablet();

static bool isMasterSP1();

static bool is9X0();

static bool isWayland();

static bool isXcb();
};

#endif // USDBASECLASS_H
