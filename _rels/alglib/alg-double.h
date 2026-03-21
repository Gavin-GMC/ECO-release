#ifndef ALG_DOUBLE_H
#define ALG_DOUBLE_H
#include<algorithm>

namespace alglib {
    const double EQACCURACY = 1e-10; // 默认精度

    // 两个浮点值在精度内是否相等
    bool equal(double a, double b, double accuracy = EQACCURACY)
    {
        return abs(a - b) <= accuracy;
    }

    // 两个浮点值在精度内前值是否大于后值
    bool large(double a, double b, double accuracy = EQACCURACY)
    {
        return a - b > accuracy;
    }

    // 两个浮点值在精度内前值是否小于后值
    bool less(double a, double b, double accuracy = EQACCURACY)
    {
        return b - a > accuracy;
    }

    // 两个浮点值在精度内前值是否不大于后值
    bool notlarge(double a, double b, double accuracy = EQACCURACY)
    {
        return a - b <= accuracy;
    }

    // 两个浮点值在精度内前值是否不小于后值
    bool notless(double a, double b, double accuracy = EQACCURACY)
    {
        return b - a <= accuracy;
    }

    // 消除double精度导致取整误差(有问题！！！)
    int intelliTrunc(double value, double accuracy = EQACCURACY)
    {
        return int(value + accuracy);
    }
}

#endif // !ALG_DOUBLE_H



