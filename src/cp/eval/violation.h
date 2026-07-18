#pragma once
//
// cp/eval/violation.h
//
// 模式 B：违反程度评估。要求所有变量已赋为点值（单点域）；否则报错（usage error）。
// 每条约束按 ε 容忍带度量违反，系统违反 = Σ weightᵢ · violationᵢ（不归一化）。
//   x<=y / x<y : max(0, x − y − ε)
//   x>=y / x>y : max(0, y − x − ε)
//   x==y       : max(0, |x − y| − ε)
//   x!=y       : (|x − y| <= ε) ? ne_penalty : 0
//
#include <vector>
#include <cmath>
#include <stdexcept>
#include "cp/compile/program.h"
#include "cp/domain/variable_domain.h"

namespace ECFlow {

struct ViolationReport {
    double              total = 0.0;   // 加权总违反
    std::vector<double> per_constraint; // 每条约束的（未加权）违反
};

class usage_error : public std::logic_error {
public:
    explicit usage_error(const std::string& m) : std::logic_error(m) {}
};

// 取节点在「全部变量为点值」下的标量值（区间中点；外向取整带来的微宽可忽略）。
inline double node_value(State& st, NodeId id) {
    const interval_set& s = st.forward(id);
    if (s.is_empty()) throw usage_error("node value undefined (empty domain)");
    return 0.5 * (s.min() + s.max());
}

inline ViolationReport violation(State& st) {
    const Program& prog = st.program();
    const Config&  cfg  = st.config();
    const double   e    = cfg.eps;

    // 前置条件：所有变量为单点
    for (std::size_t slot = 0; slot < prog.variable_count(); ++slot) {
        const interval_set& d = st.domain((int32_t)slot);
        if (d.is_empty() || d.count() != 1 || (d.max() - d.min()) > e)
            throw usage_error("violation() requires every variable assigned to a point value: '"
                              + prog.variable_name((int32_t)slot) + "'");
    }

    ViolationReport rep;
    rep.per_constraint.reserve(prog.constraints().size());
    for (const auto& c : prog.constraints()) {
        const Node& n = prog.node(c.root);
        const double x = node_value(st, n.a);
        const double y = node_value(st, n.b);
        double v = 0.0;
        switch (n.op) {
            case Op::Lt: case Op::Le: v = std::max(0.0, x - y - e); break;
            case Op::Gt: case Op::Ge: v = std::max(0.0, y - x - e); break;
            case Op::Eq:              v = std::max(0.0, std::fabs(x - y) - e); break;
            case Op::Ne:              v = (std::fabs(x - y) <= e) ? cfg.ne_penalty : 0.0; break;
            default: break;
        }
        rep.per_constraint.push_back(v);
        rep.total += c.weight * v;
    }
    return rep;
}

} // namespace ECFlow
