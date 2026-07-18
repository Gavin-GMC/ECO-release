#pragma once
//
// cp/eval/propagate.h
//
// 模式 A：反向约束传播 + AC3 不动点。把「每条约束必须成立」沿表达式 DAG 逆向
// 收窄各变量可行域，迭代到不动点。
//
// 逆向规则复用正向集合运算（自动继承符号分区凸包近似，且健全）：
//   z = x + y  ⟹  x ∈ z − y,   y ∈ z − x
//   z = x − y  ⟹  x ∈ z + y,   y ∈ x − z
//   z = x * y  ⟹  x ∈ z / y,   y ∈ z / x
//   z = x / y  ⟹  x ∈ z * y,   y ∈ x / z
//   z = −x     ⟹  x ∈ −z
//   关系(强制为真)：
//     x<y / x<=y ⟹ x.upper←y.max, y.lower←x.min
//     x>y / x>=y ⟹ x.lower←y.min, y.upper←x.max
//     x==y       ⟹ x ∈ dom(y),  y ∈ dom(x)
//     x!=y       ⟹ 仅当两侧同为相等单点时判不可行（连续域上挖点为零测度）
//
// 终止：变量域单调收缩、下有空域为界；边界移动量 < delta 不再入队，避免无限微缩。
//
#include <vector>
#include <deque>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "cp/compile/program.h"
#include "cp/domain/variable_domain.h"

namespace ECFlow {

struct SolveReport {
    bool   feasible   = true;   // 是否存在可行域（无变量域被收空）
    int    culprit    = -1;     // 致空的约束下标（feasible=false 时有效）
    int    iterations = 0;      // revise 次数
};

class Propagator {
public:
    explicit Propagator(State& st)
        : st_(st), prog_(st.program()),
          changed_mark_(st.program().variable_count(), 0) {}

    // force_full=true 强制全量重扫；否则首次 solve 全量、之后只 seed 脏变量涉及的约束。
    SolveReport run(bool force_full = false) {
        const auto& cons = prog_.constraints();
        std::deque<int> q;
        std::vector<char> inq(cons.size(), (char)0);
        auto enqueue = [&](int c) { if (!inq[c]) { inq[c] = (char)1; q.push_back(c); } };

        if (force_full || !st_.ever_settled()) {
            for (int c = 0; c < (int)cons.size(); ++c) enqueue(c);
        } else {
            for (int slot : st_.dirty_slots())
                for (int cc : prog_.constraints_of(slot)) enqueue(cc);
        }

        SolveReport rep;
        const std::uint64_t hard_cap =
            (std::uint64_t)(cons.size() + 1) * (prog_.node_count() + 1) * 64 + 1024;

        while (!q.empty()) {
            int c = q.front(); q.pop_front(); inq[c] = 0;
            unmark_all(); changed_list_.clear();
            bool ok = backward(cons[c].root, one_set());
            ++rep.iterations;
            if (!ok) { rep.feasible = false; rep.culprit = c; break; }
            for (int slot : changed_list_)
                for (int cc : prog_.constraints_of(slot)) enqueue(cc);
            if ((std::uint64_t)rep.iterations > hard_cap) break; // 安全阀
        }

        st_.clear_dirty();
        st_.mark_settled();
        return rep;
    }

private:
    State&         st_;
    const Program& prog_;
    std::vector<char>             changed_mark_;
    std::vector<int>              changed_list_;

    static double inf() { return std::numeric_limits<double>::infinity(); }
    static interval_set one_set()  { return interval_set::singleton(1.0); }
    static interval_set seg(double lo, double hi) { return interval_set(ECFlow::interval(lo, hi)); }

    void mark_changed(int slot) {
        if (!changed_mark_[slot]) { changed_mark_[slot] = 1; changed_list_.push_back(slot); }
    }
    void unmark_all() { for (int s : changed_list_) changed_mark_[s] = 0; }

    // 逆向收窄节点 id 到 target，递归到叶子。返回是否仍可行（非空）。
    bool backward(NodeId id, const interval_set& target) {
        interval_set cur = st_.forward(id).intersect(target);
        if (cur.is_empty()) return false;

        const Node& n = prog_.node(id);
        switch (n.op) {
            case Op::Const:
                return true;                       // 常量不可收窄（cur 已非空）

            case Op::Var: {
                int slot = n.slot;
                bool sig = st_.narrow(slot, cur);
                if (st_.domain(slot).is_empty()) return false;
                if (sig) mark_changed(slot);
                return true;
            }

            case Op::Neg:
                return backward(n.a, cur.negate());

            case Op::Add: {
                interval_set X = st_.forward(n.a), Y = st_.forward(n.b);
                return backward(n.a, cur.sub(Y)) && backward(n.b, cur.sub(X));
            }
            case Op::Sub: {
                interval_set X = st_.forward(n.a), Y = st_.forward(n.b);
                return backward(n.a, cur.add(Y)) && backward(n.b, X.sub(cur));
            }
            case Op::Mul: {
                interval_set X = st_.forward(n.a), Y = st_.forward(n.b);
                return backward(n.a, cur.div(Y)) && backward(n.b, cur.div(X));
            }
            case Op::Div: {
                interval_set X = st_.forward(n.a), Y = st_.forward(n.b);
                return backward(n.a, cur.mul(Y)) && backward(n.b, X.div(cur));
            }

            // 关系：cur ⊆ {0,1}。约束传播总是要求为真（target={1}）。
            case Op::Lt: case Op::Le:
            case Op::Gt: case Op::Ge:
            case Op::Eq: case Op::Ne:
                return backward_relational(n, cur);

            default:
                return true;
        }
    }

    bool backward_relational(const Node& n, const interval_set& cur) {
        const bool can_true  = cur.contains(1.0);
        const bool can_false = cur.contains(0.0);
        // 既可真又可假 -> 无法定向收窄；非真非假已在上层判空。
        if (can_true && can_false) return true;

        interval_set X = st_.forward(n.a), Y = st_.forward(n.b);
        const double xmin = X.min(), xmax = X.max(), ymin = Y.min(), ymax = Y.max();

        if (can_true) {
            switch (n.op) {
                case Op::Lt: case Op::Le:
                    return backward(n.a, seg(-inf(), ymax)) && backward(n.b, seg(xmin, inf()));
                case Op::Gt: case Op::Ge:
                    return backward(n.a, seg(ymin, inf())) && backward(n.b, seg(-inf(), xmax));
                case Op::Eq:
                    return backward(n.a, Y) && backward(n.b, X);
                case Op::Ne:
                    if (X.count() == 1 && Y.count() == 1 &&
                        X.min() == X.max() && Y.min() == Y.max() &&
                        std::fabs(X.min() - Y.min()) <= st_.config().eps)
                        return false;            // 两侧同为相等单点 -> 不可行
                    return true;
                default: return true;
            }
        } else { // 强制为假（对称情形，供完整性）
            switch (n.op) {
                case Op::Lt: case Op::Le:    // !(x<y) => x>=y
                    return backward(n.a, seg(ymin, inf())) && backward(n.b, seg(-inf(), xmax));
                case Op::Gt: case Op::Ge:    // !(x>y) => x<=y
                    return backward(n.a, seg(-inf(), ymax)) && backward(n.b, seg(xmin, inf()));
                case Op::Eq:                 // x!=y
                    return true;
                case Op::Ne:                 // x==y
                    return backward(n.a, Y) && backward(n.b, X);
                default: return true;
            }
        }
    }
};

// 便捷入口：原地把 st 的变量域传播到不动点。
// force_full=true 时强制全量重扫（默认增量：首次全量、之后只处理脏变量涉及的约束）。
inline SolveReport propagate(State& st, bool force_full = false) {
    Propagator p(st);
    return p.run(force_full);
}

} // namespace ECFlow
