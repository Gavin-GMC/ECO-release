#pragma once
//
// cp/core/interval_set.h
//
// 多区间集合：有序、互不相交、相邻即合并的 interval 列表，表示一个变量域
// 或一个表达式的结果域（可含间隙）。
//
// 关键算法（详见设计记忆 constraint-lib-design）：
//  - 加 / 减：Minkowski 组合，**保留间隙**（两两组合后插入并自动合并）。
//  - 乘 / 除：**符号分区凸包近似** —— 把每个操作数压成 ≤2 段（按 0 分割的
//    负侧凸包 + 正侧凸包），再两两组合，扇出 ≤4。外包络精确、跨零间隙保留，
//    只丢同号内部洞（该误差在后续乘除中自衰减）。
//  - remove：挖洞（!= / 显式排除 / 除法间隙）。
//  - collapse_to：碎片数超 max_intervals 时，优先填「乘性比值最小」的同号洞，
//    跨零洞绝不塌缩，纯内存兜底。
//
// 本类是「纯值类型」，不含版本号/时间戳；增量所需的版本信息由 State 维护。
//
#include <algorithm>
#include <string>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include "cp/core/interval.h"
#include "cp/core/small_vector.h"

namespace ECFlow {

template <class Traits = default_scalar_traits, std::size_t InlineN = 8>
class basic_interval_set {
public:
    using traits_type   = Traits;
    using value_type    = typename Traits::value_type;
    using interval_type = basic_interval<Traits>;
    using store_type    = small_vector<interval_type, InlineN>;

private:
    store_type ivs_; // 有序、不相交、不相邻（前 InlineN 段内联，零堆分配）

    // 插入一个区间并维护「有序 / 合并重叠或相邻」不变式。
    void insert_merged(interval_type iv) {
        if (iv.is_empty()) return;
        value_type lo = iv.lower();
        value_type hi = iv.upper();

        // 找到第一个 upper >= lo 的位置（可能与之合并）
        auto it = ivs_.begin();
        while (it != ivs_.end() && it->upper() < lo) ++it;

        // 向右吞并所有 lower <= hi 的区间
        while (it != ivs_.end() && it->lower() <= hi) {
            lo = std::min(lo, it->lower());
            hi = std::max(hi, it->upper());
            it = ivs_.erase(it);
        }
        ivs_.insert(it, interval_type(lo, hi));
    }

    // 把集合按 0 分割成 ≤2 段符号凸包：{负侧, 正侧}（各自可空）。
    // 负侧 = hull(set ∩ (-inf,0])，正侧 = hull(set ∩ [0,+inf])。跨零间隙得以保留。
    void sign_hulls(interval_type& neg, interval_type& pos) const {
        neg = interval_type::empty();
        pos = interval_type::empty();
        const interval_type zlo(Traits::neg_infinity(), value_type(0));
        const interval_type zhi(value_type(0), Traits::infinity());
        for (const auto& iv : ivs_) {
            neg = neg.hull(iv.intersect(zlo));
            pos = pos.hull(iv.intersect(zhi));
        }
    }

    // 符号一致区间的倒数 1/[c,d]（要求 c>=0 或 d<=0）。除零点返回空。
    static interval_type reciprocal_sign_consistent(const interval_type& d) {
        if (d.is_empty()) return interval_type::empty();
        const value_type c = d.lower(), e = d.upper();
        const value_type inf = Traits::infinity(), ninf = Traits::neg_infinity();
        if (c == value_type(0) && e == value_type(0)) return interval_type::empty(); // 1/0 undefined
        value_type rlo, rhi;
        if (c > value_type(0)) {            // [c,e] 全正
            rlo = value_type(1) / e; rhi = value_type(1) / c;
        } else if (e < value_type(0)) {     // [c,e] 全负
            rlo = value_type(1) / e; rhi = value_type(1) / c;
        } else if (c == value_type(0)) {    // [0,e], e>0  -> [1/e, +inf)
            rlo = value_type(1) / e; rhi = inf;
        } else {                            // [c,0], c<0  -> (-inf, 1/c]
            rlo = ninf; rhi = value_type(1) / c;
        }
        return interval_type(Traits::round_down(rlo), Traits::round_up(rhi));
    }

public:
    basic_interval_set() = default;

    // 单区间 / 单点 构造
    explicit basic_interval_set(const interval_type& iv) { insert_merged(iv); }
    static basic_interval_set whole()        { return basic_interval_set(interval_type::whole()); }
    static basic_interval_set singleton(value_type v) { return basic_interval_set(interval_type::singleton(v)); }

    // 从 {lo,hi} 列表构造
    basic_interval_set(std::initializer_list<std::pair<value_type, value_type>> parts) {
        for (auto& p : parts) insert_merged(interval_type(p.first, p.second));
    }

    // —— 访问 ——
    const store_type& intervals() const { return ivs_; }
    bool        is_empty() const { return ivs_.empty(); }
    std::size_t count()    const { return ivs_.size(); }

    value_type min() const { return ivs_.empty() ? Traits::infinity()     : ivs_.front().lower(); }
    value_type max() const { return ivs_.empty() ? Traits::neg_infinity() : ivs_.back().upper();  }

    bool contains(value_type v) const {
        for (const auto& iv : ivs_) {
            if (iv.contains(v)) return true;
            if (iv.lower() > v) break;
        }
        return false;
    }

    void clear() { ivs_.clear(); }
    void add_interval(const interval_type& iv) { insert_merged(iv); }

    // —— 集合运算 ——
    // 交：与另一集合求交（有序双指针）。
    basic_interval_set intersect(const basic_interval_set& o) const {
        basic_interval_set r;
        std::size_t i = 0, j = 0;
        while (i < ivs_.size() && j < o.ivs_.size()) {
            interval_type x = ivs_[i].intersect(o.ivs_[j]);
            if (!x.is_empty()) r.insert_merged(x);
            if (ivs_[i].upper() < o.ivs_[j].upper()) ++i; else ++j;
        }
        return r;
    }

    // 并：把另一集合并入。
    basic_interval_set unite(const basic_interval_set& o) const {
        basic_interval_set r = *this;
        for (const auto& iv : o.ivs_) r.insert_merged(iv);
        return r;
    }

    // —— 算术 ——
    // 加 / 减：Minkowski，保留间隙。
    basic_interval_set add(const basic_interval_set& o) const {
        basic_interval_set r;
        for (const auto& a : ivs_)
            for (const auto& b : o.ivs_)
                r.insert_merged(a.add(b));
        return r;
    }
    basic_interval_set sub(const basic_interval_set& o) const {
        basic_interval_set r;
        for (const auto& a : ivs_)
            for (const auto& b : o.ivs_)
                r.insert_merged(a.sub(b));
        return r;
    }
    basic_interval_set negate() const {
        basic_interval_set r;
        for (std::size_t i = ivs_.size(); i-- > 0; ) r.insert_merged(ivs_[i].negate());
        return r;
    }

    // 乘：符号分区凸包近似（扇出 ≤4）。
    basic_interval_set mul(const basic_interval_set& o) const {
        basic_interval_set r;
        if (is_empty() || o.is_empty()) return r;
        interval_type an, ap, bn, bp;
        sign_hulls(an, ap);
        o.sign_hulls(bn, bp);
        interval_type as[2] = {an, ap}, bs[2] = {bn, bp};
        for (auto& a : as) {
            if (a.is_empty()) continue;
            for (auto& b : bs) {
                if (b.is_empty()) continue;
                r.insert_merged(a.mul(b));
            }
        }
        return r;
    }

    // 除：符号分区凸包近似。除数跨零经 sign_hulls 自然分裂为两段 → 双射线。
    basic_interval_set div(const basic_interval_set& o) const {
        basic_interval_set r;
        if (is_empty() || o.is_empty()) return r;
        interval_type an, ap, bn, bp;
        sign_hulls(an, ap);
        o.sign_hulls(bn, bp);
        interval_type as[2] = {an, ap}, bs[2] = {bn, bp};
        for (auto& a : as) {
            if (a.is_empty()) continue;
            for (auto& b : bs) {
                if (b.is_empty()) continue;
                interval_type recip = reciprocal_sign_consistent(b);
                if (recip.is_empty()) continue; // 除以 {0}
                r.insert_merged(a.mul(recip));
            }
        }
        return r;
    }

    // —— 收紧 / 挖洞（反向传播原语）——
    // 去掉所有 < v 的部分（与 [v,+inf) 求交）。返回是否仍非空。
    bool clamp_lower(value_type v) {
        store_type out;
        for (auto& iv : ivs_) {
            if (iv.upper() < v) continue;            // 整段被切掉
            interval_type t = iv;
            t.clamp_lower(v);
            if (!t.is_empty()) out.push_back(t);
        }
        ivs_ = std::move(out);
        return !ivs_.empty();
    }
    // 去掉所有 > v 的部分（与 (-inf,v] 求交）。
    bool clamp_upper(value_type v) {
        store_type out;
        for (auto& iv : ivs_) {
            if (iv.lower() > v) break;
            interval_type t = iv;
            t.clamp_upper(v);
            if (!t.is_empty()) out.push_back(t);
        }
        ivs_ = std::move(out);
        return !ivs_.empty();
    }

    // 挖掉开区间内容 (lo, hi) —— 落在 [lo,hi] 内的部分被移除，可能把一段劈成两段。
    // lo>hi 视为空操作。返回是否仍非空。
    bool remove(value_type lo, value_type hi) {
        if (lo > hi) return !ivs_.empty();
        store_type out;
        for (auto& iv : ivs_) {
            if (iv.upper() < lo || iv.lower() > hi) { out.push_back(iv); continue; }
            // 左残段 [iv.lo, lo]
            interval_type left = iv; left.clamp_upper(lo);
            if (!left.is_empty()) out.push_back(left);
            // 右残段 [hi, iv.hi]
            interval_type right = iv; right.clamp_lower(hi);
            if (!right.is_empty()) out.push_back(right);
        }
        ivs_ = std::move(out);
        return !ivs_.empty();
    }

    // —— 塌缩（内存兜底）——
    // 若碎片数 > max_n，反复合并「代价最小」的间隙直到 <= max_n。
    // 代价 = 同号间隙的乘性比值（越接近 1 越优先填）；跨零 / 触零间隙代价视为 +inf，
    // 除非已无可选才退化为按绝对宽度填。
    void collapse_to(std::size_t max_n) {
        if (max_n < 1) max_n = 1;
        while (ivs_.size() > max_n) {
            std::size_t best = 0;
            value_type best_cost = Traits::infinity();
            bool found_finite = false;
            value_type best_width = Traits::infinity();
            std::size_t best_width_idx = 0;
            for (std::size_t i = 0; i + 1 < ivs_.size(); ++i) {
                const value_type u = ivs_[i].upper();
                const value_type l = ivs_[i + 1].lower();
                const value_type gap_w = l - u;
                if (gap_w < best_width) { best_width = gap_w; best_width_idx = i; }
                value_type cost;
                if (u < value_type(0) && l > value_type(0)) {
                    cost = Traits::infinity();                 // 跨零，保护
                } else if (u > value_type(0)) {
                    cost = l / u;                              // 同正
                } else if (l < value_type(0)) {
                    cost = u / l;                              // 同负
                } else {
                    cost = Traits::infinity();                 // 触零，保护
                }
                if (cost < best_cost) { best_cost = cost; best = i; found_finite = true; }
            }
            std::size_t idx = found_finite ? best : best_width_idx;
            interval_type merged = ivs_[idx].hull(ivs_[idx + 1]);
            ivs_.erase(ivs_.begin() + idx, ivs_.begin() + idx + 2);
            ivs_.insert(ivs_.begin() + idx, merged);
        }
    }

    std::string to_string() const {
        if (ivs_.empty()) return "[]";
        std::string s = ivs_.front().to_string();
        for (std::size_t i = 1; i < ivs_.size(); ++i) s += " U " + ivs_[i].to_string();
        return s;
    }

    friend bool operator==(const basic_interval_set& a, const basic_interval_set& b) {
        return a.ivs_ == b.ivs_;
    }
    friend bool operator!=(const basic_interval_set& a, const basic_interval_set& b) { return !(a == b); }
};

using interval_set = basic_interval_set<default_scalar_traits>;

} // namespace ECFlow
