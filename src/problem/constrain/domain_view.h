//------------------------Description------------------------
// domain_view:连续域 interval_set 之上的「变量域视图」。**双模式**——
//   grid(网格/离散族) / measure(测度/连续族),接口对齐、由句柄按变量类型选定。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include "cp/core/interval_set.h"
#include "ecflow-rand.h"      // ECFlow::wide_rand, ECFlow::rand01
#include "ecflow-constant.h"  // EMPTYVALUE
#include "variable.h"       // VariableType(供 viewModeOf 翻译"变量类型 → 视图模式")

namespace ECFlow {

// getFeasibleList 返回枚举点的全局上限：当某维真实可行网格点数超过它时，列表被
// 降采样为至多该数量、在可行域内尽量均匀分布的点（端点包含）。仅作用于
// getFeasibleList 路径，不影响启发式/累积约束的全枚举。<=0 表示不限。可运行期修改。
// measure 模式下它同时是**枚举/计数的采样点数**（连续域无"真实点数"可言）。
inline int FEASIBLE_LIST_LIMIT = 1024;

// 域的解释方式。由句柄按变量类型选定（VariableType::continuous → measure，其余 → grid）。
enum class ViewMode { grid, measure };

// **变量类型 → 视图模式**:全平台构造 domain_view 时一律经此翻译,勿在别处硬写 mode。
//   视图本身不认识 VariableType(那是问题定义层的概念);本函数是二者之间唯一的桥。
inline ViewMode viewModeOf(VariableType t)
{
    return t == VariableType::continuous ? ViewMode::measure : ViewMode::grid;
}

// 非拥有视图：绑定到外部 interval_set + 该变量的网格/精度 (lowbound, accuracy) + 解释模式。
// 必须保证 set 的生命周期长于本视图。
struct domain_view {
    interval_set& set;
    double        lowbound;
    double        accuracy;   // grid:网格步长   measure:输出精度
    ViewMode      mode;

    domain_view(interval_set& s, double lb, double acc, ViewMode m = ViewMode::grid)
        : set(s), lowbound(lb), accuracy(acc), mode(m) {}

    bool is_measure() const { return mode == ViewMode::measure; }

    // 可行域总测度（各区间长度之和）。measure 模式的"大小"度量；grid 模式一般不用。
    double measure() const {
        double m = 0;
        for (const auto& iv : set.intervals())
            if (!iv.is_empty()) m += iv.upper() - iv.lower();
        return m;
    }

    // 把任意值落到该变量的合法取值上（不检查是否在域内）。**全平台唯一的"值→取值"落点**
    //   。
    //
    // ★ measure(连续)→ **原值返回,不做任何离散化**。连续变量的合法取值就是实数本身,没有理由被量化。
    //   accuracy 在此仅表达"多细算同一个值"(供 remove_point 界定"删掉一个值"的邻域、及输出/展示),
    //   **不作用于存储值** —— 若在此量化,连续变量就又被压回网格,measure 模式也就白设了。
    //
    // ★ grid(离散)→ 吸附到**最近**的网格点 lowbound + k*accuracy(就近舍入)。
    //   ⑴**去偏置**:商 `(v-lowbound)/accuracy` **恒非负** → 截断向零实为 **floor**。设提议值 = 格点 k + 噪声,
    //      则 `+0.6·acc → trunc(k+0.6)=k`(移动 **0**)、`-0.6·acc → trunc(k-0.6)=k-1`(移动 **-1·acc**)
    //      —— 正噪声被吃掉、负噪声被放大,每次离散化平均把值往**下界**拉 `0.5·acc`,
    //      **打破 Gauss/PM/DE/PSO 等对称噪声算子的对称性**。就近则 `±0.6 → k±1`,无偏。
    //   ⑵**误差减半**:max |v - snap(v)| 由 `1.0·acc` 降至 `0.5·acc`。
    //   ⑶**去掉一个补丁**:原实现的浮点噪声特判(`isIntegerInPrecision(x) ? round(x) : trunc(x)`)
    //      存在的唯一理由是让 `2.9999999999 → 3`(截断会给 2,差一格);而 llround **天然**给 3
    //      —— 那个特判本就是截断的症状,改就近后随之消失。
    //   注:llround 返回 long long(值域 9e18),细步长下的商(如 [-100,100]@1e-8 约 2e10)远未触及。
    double snap(double v) const {
        if (is_measure()) return v;   // 连续:不离散化,原值返回
        long long k = std::llround((v - lowbound) / accuracy);
        return lowbound + static_cast<double>(k) * accuracy;
    }

    // 区间 [a,b] 内的网格下标范围 [k_lo, k_hi]（含端点；空则 k_lo > k_hi）。
    void grid_range(double a, double b, long long& k_lo, long long& k_hi) const {
        k_lo = static_cast<long long>(std::ceil ((a - lowbound) / accuracy - 1e-9));
        k_hi = static_cast<long long>(std::floor((b - lowbound) / accuracy + 1e-9));
    }

    // 可行"选择"数。
    //   grid    : 可行网格点总数。
    //   measure : **点采样退化** —— 连续域无真实点数;返回与 enumerate() 一致的采样点数,
    //             二者必须自洽(历史教训:ACS 曾用 getChoiceNumber 当 getFeasibleList 的长度 → 越界)。
    int count() const {
        if (set.is_empty()) return 0;
        if (is_measure()) return measure() > 0 ? FEASIBLE_LIST_LIMIT : 0;
        long long n = 0;
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
            if (khi >= klo) n += (khi - klo + 1);
        }
        return static_cast<int>(n);
    }

    // 按**测度**均匀取第 i/(n-1) 个分位点（i∈[0,n-1]）。measure 模式的采样基元。
    double measure_at(int i, int n) const {
        double total = measure();
        if (total <= 0 || n <= 0) return EMPTYVALUE;
        double t = (n == 1) ? 0.0 : static_cast<double>(i) / (n - 1);   // 含两端点
        double target = t * total;
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            double len = iv.upper() - iv.lower();
            if (target <= len) return iv.lower() + target;
            target -= len;
        }
        return set.intervals().back().upper();   // 浮点兜底：落在最右端
    }

    // 枚举可行"选择"（升序）。
    //   grid    : 全部可行网格点。
    //   measure : **点采样退化** —— 按测度均匀取 FEASIBLE_LIST_LIMIT 个点（含两端点）。
    std::vector<double> enumerate() const {
        std::vector<double> out;
        if (is_measure()) {
            int n = count();
            out.reserve(n);
            for (int i = 0; i < n; ++i) out.push_back(measure_at(i, n));
            return out;
        }
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
            for (long long k = klo; k <= khi; ++k)
                out.push_back(lowbound + static_cast<double>(k) * accuracy);
        }
        return out;
    }

    // 枚举到新分配数组（调用方负责 delete[]）。等价 FeasibleLine::getFeasibleList(int&)。
    double* enumerate_alloc(int& size) const {
        std::vector<double> v = enumerate();
        size = static_cast<int>(v.size());
        double* out = new double[size > 0 ? size : 1];
        for (int i = 0; i < size; ++i) out[i] = v[i];
        return out;
    }

    // 第 gidx 个可行"选择"（跨所有段、0 基）；越界返回 EMPTYVALUE。
    double value_at(long long gidx) const {
        if (is_measure()) {
            int n = count();
            if (gidx < 0 || gidx >= n) return EMPTYVALUE;
            return measure_at(static_cast<int>(gidx), n);
        }
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
            long long cnt = (khi >= klo) ? (khi - klo + 1) : 0;
            if (gidx < cnt) return lowbound + static_cast<double>(klo + gidx) * accuracy;
            gidx -= cnt;
        }
        return EMPTYVALUE;
    }

    // 同 enumerate_alloc，但把返回点数上限设为 limit：真实点数 > limit 时，返回 limit
    // 个在 [0, total-1] 上等距取样（含两端点）的点，尽量均匀覆盖可行域。limit<=0 不限。
    //   measure 模式下 count() 已是采样数，故当 limit >= 它时直接走 enumerate_alloc。
    double* enumerate_alloc_capped(int limit, int& size) const {
        long long total = count();
        if (limit <= 0 || total <= limit) return enumerate_alloc(size);
        size = limit;
        double* out = new double[limit];
        for (int i = 0; i < limit; ++i) {
            long long gidx = (limit == 1) ? 0
                           : static_cast<long long>(i) * (total - 1) / (limit - 1);
            out[i] = value_at(gidx);
        }
        return out;
    }

    // 随机取一个可行值；域空返回 EMPTYVALUE。**热路径**(getRandomChoiceInspace)。
    //   grid    : 可行网格点**等概率**。
    //   measure : 按**长度测度**均匀 —— 直接按测度抽,**不经 count()/枚举**(故连续域上零退化、
    //             亦无"为取一个随机数先数遍 2e7 个格点"的问题)。
    double random() const {
        if (is_measure()) {
            double total = measure();
            if (total <= 0) return EMPTYVALUE;
            double target = rand01() * total;
            for (const auto& iv : set.intervals()) {
                if (iv.is_empty()) continue;
                double len = iv.upper() - iv.lower();
                if (target <= len) return iv.lower() + target;
                target -= len;
            }
            return set.intervals().back().upper();   // 浮点兜底
        }
        int n = count();
        if (n <= 0) return EMPTYVALUE;
        int target = static_cast<int>(wide_rand() % n);
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
            int cnt = (khi >= klo) ? static_cast<int>(khi - klo + 1) : 0;
            if (target < cnt) return lowbound + static_cast<double>(klo + target) * accuracy;
            target -= cnt;
        }
        return EMPTYVALUE;
    }

    // 取最左/最右可行值；域空返回 EMPTYVALUE。
    //   measure 下即区间端点本身(无需吸附到格点)。
    double boundary(bool left = true) const {
        if (set.is_empty()) return EMPTYVALUE;
        const auto& iv = left ? set.intervals().front() : set.intervals().back();
        if (is_measure()) return left ? iv.lower() : iv.upper();
        long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
        if (khi < klo) return EMPTYVALUE;
        return lowbound + static_cast<double>(left ? klo : khi) * accuracy;
    }

    // 域内距 v 最近的可行值；域空返回 EMPTYVALUE。
    //   measure 下把 v 钳到最近区间(不吸附格点)。
    double closest(double v) const {
        double best = EMPTYVALUE, bestd = 1e300;
        if (is_measure()) {
            for (const auto& iv : set.intervals()) {
                if (iv.is_empty()) continue;
                double cand = v < iv.lower() ? iv.lower() : (v > iv.upper() ? iv.upper() : v);
                double d = std::fabs(cand - v);
                if (d < bestd) { bestd = d; best = cand; }
            }
            return best;
        }
        long long kv = static_cast<long long>(std::llround((v - lowbound) / accuracy));
        for (const auto& iv : set.intervals()) {
            if (iv.is_empty()) continue;
            long long klo, khi; grid_range(iv.lower(), iv.upper(), klo, khi);
            if (khi < klo) continue;
            long long kc = kv < klo ? klo : (kv > khi ? khi : kv);
            double cand = lowbound + static_cast<double>(kc) * accuracy;
            double d = std::fabs(cand - v);
            if (d < bestd) { bestd = d; best = cand; }
        }
        return best;
    }

    bool contains_value(double v) const { return set.contains(v); }

    // 删除值 v（半步胞间隙）。两模式**同实现**，语义各自成立（见文件头）。
    void remove_point(double v) {
        const double half = 0.5 * accuracy;
        set.remove(v - half, v + half);
    }

    // 收窄到连续区间 [l, r]（与之求交）。等价 FeasibleLine::unite(l,r,...)。两模式同实现。
    void restrict(double l, double r) {
        set = set.intersect(interval_set(interval(l, r)));
    }
};

// 由变量上下界/精度构造「初始满域」interval_set（等价 FeasibleLine(ElementNote*)）。
inline interval_set make_domain(double lowbound, double upbound) {
    return interval_set(interval(lowbound, upbound));
}

}
