#pragma once
//
// cp/core/interval.h
//
// 单区间 [lo, hi]，闭区间，端点可为 ±inf。
//
// 约定：
//  - 空集规范表示为 lo > hi（canonical empty = [+inf, -inf]）。不另设 bool 标志。
//  - 单点 [v, v] 即 lo == hi。
//  - 所有算术结果端点经 Traits 外向取整，保证结果区间一定包含真实值集合（健全）。
//  - 乘法用 ext_mul，约定 0 * (±inf) = 0，避免 NaN。
//
// 本文件只含「单区间代数」：交、并(凸包)、加、减、乘、收紧端点。
// 除法因除数跨零会分裂为多区间，放在 interval_set.h 处理。
//
#include <algorithm>
#include <string>
#include "cp/core/scalar_traits.h"

namespace ECFlow {

template <class Traits = default_scalar_traits>
class basic_interval {
public:
    using traits_type = Traits;
    using value_type  = typename Traits::value_type;

private:
    value_type lo_;
    value_type hi_;

public:
    // 默认构造 = 全集 (-inf, +inf)，作为「无约束」初值，也是交运算的单位元。
    basic_interval()
        : lo_(Traits::neg_infinity()), hi_(Traits::infinity()) {}

    basic_interval(value_type lo, value_type hi)
        : lo_(lo), hi_(hi) {}

    // —— 命名构造 ——
    static basic_interval empty()         { return basic_interval(Traits::infinity(), Traits::neg_infinity()); }
    static basic_interval whole()         { return basic_interval(Traits::neg_infinity(), Traits::infinity()); }
    static basic_interval singleton(value_type v) { return basic_interval(v, v); }

    // —— 访问 ——
    value_type lower() const { return lo_; }
    value_type upper() const { return hi_; }

    bool is_empty()     const { return lo_ > hi_; }
    bool is_whole()     const { return lo_ == Traits::neg_infinity() && hi_ == Traits::infinity(); }
    bool is_singleton() const { return !is_empty() && lo_ == hi_; }
    bool is_bounded()   const { return lo_ != Traits::neg_infinity() && hi_ != Traits::infinity(); }

    bool contains(value_type v) const { return !is_empty() && v >= lo_ && v <= hi_; }

    // 宽度；空集为 0，无界为 +inf。
    value_type width() const {
        if (is_empty()) return value_type(0);
        return hi_ - lo_;
    }

    // —— 集合关系 ——
    bool intersects(const basic_interval& o) const {
        if (is_empty() || o.is_empty()) return false;
        return !(hi_ < o.lo_ || lo_ > o.hi_);
    }

    basic_interval intersect(const basic_interval& o) const {
        if (is_empty() || o.is_empty()) return empty();
        return basic_interval(std::max(lo_, o.lo_), std::min(hi_, o.hi_)); // lo>hi 时自然为空
    }

    // 凸包：把两个区间(及其间隙)并成一个区间。空集视为单位元。
    basic_interval hull(const basic_interval& o) const {
        if (is_empty())   return o;
        if (o.is_empty()) return *this;
        return basic_interval(std::min(lo_, o.lo_), std::max(hi_, o.hi_));
    }

    // —— 端点收紧（反向传播原语）——
    // 与 [v, +inf] 求交：把下界抬到 v。返回收紧后是否非空。
    bool clamp_lower(value_type v) {
        if (is_empty()) return false;
        if (v > hi_) { *this = empty(); return false; }
        if (v > lo_) lo_ = v;
        return true;
    }
    // 与 [-inf, v] 求交：把上界压到 v。返回收紧后是否非空。
    bool clamp_upper(value_type v) {
        if (is_empty()) return false;
        if (v < lo_) { *this = empty(); return false; }
        if (v < hi_) hi_ = v;
        return true;
    }

    // —— 算术（外向取整）——
    basic_interval add(const basic_interval& o) const {
        if (is_empty() || o.is_empty()) return empty();
        return basic_interval(Traits::round_down(lo_ + o.lo_),
                              Traits::round_up  (hi_ + o.hi_));
    }
    basic_interval sub(const basic_interval& o) const {
        if (is_empty() || o.is_empty()) return empty();
        return basic_interval(Traits::round_down(lo_ - o.hi_),
                              Traits::round_up  (hi_ - o.lo_));
    }
    basic_interval mul(const basic_interval& o) const {
        if (is_empty() || o.is_empty()) return empty();
        const value_type a = ext_mul(lo_, o.lo_);
        const value_type b = ext_mul(lo_, o.hi_);
        const value_type c = ext_mul(hi_, o.lo_);
        const value_type d = ext_mul(hi_, o.hi_);
        return basic_interval(Traits::round_down(std::min({a, b, c, d})),
                              Traits::round_up  (std::max({a, b, c, d})));
    }
    basic_interval negate() const {
        if (is_empty()) return empty();
        return basic_interval(-hi_, -lo_);
    }

    // —— 比较 ——
    friend bool operator==(const basic_interval& x, const basic_interval& y) {
        if (x.is_empty() && y.is_empty()) return true;
        return x.lo_ == y.lo_ && x.hi_ == y.hi_;
    }
    friend bool operator!=(const basic_interval& x, const basic_interval& y) { return !(x == y); }

    std::string to_string() const {
        if (is_empty()) return "[]";
        return "[" + std::to_string(lo_) + ", " + std::to_string(hi_) + "]";
    }
};

using interval = basic_interval<default_scalar_traits>;

} // namespace ECFlow
