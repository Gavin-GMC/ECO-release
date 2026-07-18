#pragma once
//
// cp/core/scalar_traits.h
//
// 标量类型策略：定义区间端点使用的数值类型、无穷、外向取整(directed rounding)
// 以及若干在无穷/零处避免 NaN 的扩展运算。
//
// 设计要点：
//  - 区间端点外向取整保证「健全性」：下界向下取整、上界向上取整，
//    使浮点舍入永远把区间撑大、绝不缩小，从而绝不漏掉可行解。
//  - 默认 strict_double_traits 在每次算术后按 1 ULP 外向微调(std::nextafter)，
//    代价可忽略(~2e-16 相对)，但保证结果一定包含真实值。
//  - 若追求极致速度且可容忍舍入，可改用 fast_double_traits(取整为恒等)。
//  - ext_mul：约定 0 * (±inf) = 0(乘法零吸收)，避免 IEEE 的 0*inf=NaN。
//
#include <limits>
#include <cmath>

namespace ECFlow {

// 默认：double + 1 ULP 外向取整(健全)
struct strict_double_traits {
    using value_type = double;

    static constexpr value_type infinity()     { return std::numeric_limits<value_type>::infinity(); }
    static constexpr value_type neg_infinity()  { return -std::numeric_limits<value_type>::infinity(); }
    static constexpr value_type quiet_nan()     { return std::numeric_limits<value_type>::quiet_NaN(); }

    // 把刚算出的下界向下(更小)外推一个 ULP；±inf / NaN 原样返回。
    static value_type round_down(value_type x) {
        if (!std::isfinite(x)) return x;
        return std::nextafter(x, neg_infinity());
    }
    // 把刚算出的上界向上(更大)外推一个 ULP。
    static value_type round_up(value_type x) {
        if (!std::isfinite(x)) return x;
        return std::nextafter(x, infinity());
    }
};

// 快速：double，不做外向取整(放弃严格健全性，换速度)
struct fast_double_traits {
    using value_type = double;

    static constexpr value_type infinity()     { return std::numeric_limits<value_type>::infinity(); }
    static constexpr value_type neg_infinity()  { return -std::numeric_limits<value_type>::infinity(); }
    static constexpr value_type quiet_nan()     { return std::numeric_limits<value_type>::quiet_NaN(); }

    static value_type round_down(value_type x) { return x; }
    static value_type round_up(value_type x)   { return x; }
};

using default_scalar_traits = strict_double_traits;

// 扩展乘法：约定 0 * (±inf) = 0，避免 NaN。其余情况为普通乘法。
template <class T>
inline T ext_mul(T a, T b) {
    if (a == T(0) || b == T(0)) return T(0);
    return a * b;
}

} // namespace ECFlow
