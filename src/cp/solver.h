#pragma once
//
// cp/solver.h
//
// 对外门面：把三种计算模式收口为简洁入口。
//   模式 A：solve(state)            -> SolveReport，原地把变量域传播到不动点
//   模式 B：violation(state)        -> ViolationReport（要求变量皆为点值）
//   模式 C：value_domain(state, id) -> 任意节点的当前结果域；node_value 取点值
//
#include "cp/eval/propagate.h"
#include "cp/eval/violation.h"

namespace ECFlow {

// 模式 A（增量：首次全量，之后只重传播自上次以来改动过的变量所涉约束）
inline SolveReport solve(State& st, bool force_full = false) { return propagate(st, force_full); }

// 模式 C：节点结果域（含约束根 / 中间导出量）
inline const interval_set& value_domain(State& st, NodeId id) { return st.forward(id); }

// 便捷：取第 i 条约束左/右子表达式的结果域
inline const interval_set& constraint_lhs_domain(State& st, std::size_t ci) {
    return st.forward(st.program().node(st.program().constraints()[ci].root).a);
}
inline const interval_set& constraint_rhs_domain(State& st, std::size_t ci) {
    return st.forward(st.program().node(st.program().constraints()[ci].root).b);
}

} // namespace ECFlow
