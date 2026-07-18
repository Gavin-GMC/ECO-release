#pragma once
//
// cp/domain/variable_domain.h
//
// State：绑定到某个 Program 的「变量域 + 节点缓存 + 版本号」运行时状态。
// 廉价、可换：同一个 Program 配不同 State 即可零重编译地切换变量域。
//
// 增量正向求值：用单调时钟 clock_ 给每次变量域变化盖戳；节点缓存记录自己是
// 在哪个戳算出来的，只有当某个输入的戳更新时才重算（历史实现 timestamp 思路，
// 但戳改为存在 State 而非 Node，从而 Program 只读可共享）。
//
#include <vector>
#include <cstdint>
#include <cmath>
#include <utility>
#include "cp/core/config.h"
#include "cp/core/interval_set.h"
#include "cp/compile/program.h"

namespace ECFlow {

class State {
public:
    explicit State(const Program& prog, const Config& cfg = {})
        : prog_(&prog), cfg_(cfg),
          var_dom_(prog.variable_count(), interval_set::whole()),
          var_stamp_(prog.variable_count(), 1),
          node_val_(prog.node_count()),
          node_stamp_(prog.node_count(), 0),
          dirty_mark_(prog.variable_count(), 0) {}

    const Program& program() const { return *prog_; }
    const Config&  config()  const { return cfg_; }

    // —— 设定变量域 ——
    void set_domain(int32_t slot, const interval_set& dom) {
        var_dom_[slot] = dom;
        var_dom_[slot].collapse_to(cfg_.max_intervals);
        var_stamp_[slot] = ++clock_;
        mark_dirty(slot);
    }
    void assign(int32_t slot, double v) { set_domain(slot, interval_set::singleton(v)); }

    const interval_set& domain(int32_t slot) const { return var_dom_[slot]; }

    // 与 target 求交收窄；返回是否真的变小了（供不动点判定）。
    bool narrow(int32_t slot, const interval_set& target) {
        interval_set nd = var_dom_[slot].intersect(target);
        if (nd == var_dom_[slot]) return false;
        nd.collapse_to(cfg_.max_intervals);
        var_dom_[slot] = std::move(nd);
        var_stamp_[slot] = ++clock_;
        mark_dirty(slot);
        return true;
    }

    // —— 增量传播支持：脏变量集（自上次传播稳定后被改动过的 slot）——
    const std::vector<int>& dirty_slots() const { return dirty_list_; }
    bool ever_settled() const { return ever_settled_; }
    void clear_dirty() {
        for (int s : dirty_list_) dirty_mark_[s] = 0;
        dirty_list_.clear();
    }
    void mark_settled() { ever_settled_ = true; }

    // —— 增量正向求值：返回节点 id 的当前结果域（带 memo）——
    const interval_set& forward(NodeId id) {
        const Node& n = prog_->node(id);
        switch (n.op) {
            case Op::Const:
                if (node_stamp_[id] == 0) {
                    node_val_[id] = interval_set::singleton(n.value);
                    node_stamp_[id] = 1;
                }
                return node_val_[id];

            case Op::Var:
                if (node_stamp_[id] < var_stamp_[n.slot]) {
                    node_val_[id] = var_dom_[n.slot];
                    node_stamp_[id] = var_stamp_[n.slot];
                }
                return node_val_[id];

            case Op::Neg: {
                const interval_set& A = forward(n.a);
                std::uint64_t s = node_stamp_[n.a];
                if (node_stamp_[id] < s) {
                    node_val_[id] = A.negate();
                    node_stamp_[id] = s;
                }
                return node_val_[id];
            }

            default: { // 二元
                const interval_set& A = forward(n.a);
                const interval_set& B = forward(n.b);
                std::uint64_t s = node_stamp_[n.a] > node_stamp_[n.b] ? node_stamp_[n.a] : node_stamp_[n.b];
                if (node_stamp_[id] < s) {
                    node_val_[id] = combine(n.op, A, B);
                    node_val_[id].collapse_to(cfg_.max_intervals);
                    node_stamp_[id] = s;
                }
                return node_val_[id];
            }
        }
    }

private:
    const Program* prog_;
    Config         cfg_;
    std::vector<interval_set>  var_dom_;
    std::vector<std::uint64_t> var_stamp_;
    std::vector<interval_set>  node_val_;
    std::vector<std::uint64_t> node_stamp_;
    std::uint64_t              clock_ = 1;
    std::vector<char>          dirty_mark_;
    std::vector<int>           dirty_list_;
    bool                       ever_settled_ = false;

    void mark_dirty(int32_t slot) {
        if (!dirty_mark_[slot]) { dirty_mark_[slot] = 1; dirty_list_.push_back(slot); }
    }

    interval_set combine(Op op, const interval_set& A, const interval_set& B) const {
        switch (op) {
            case Op::Add: return A.add(B);
            case Op::Sub: return A.sub(B);
            case Op::Mul: return A.mul(B);
            case Op::Div: return A.div(B);
            default:      return rel_eval(op, A, B);   // 关系：结果 ⊆ {0,1}
        }
    }

    // 关系求值：用外凸包 min/max 判定，结果域是 {0} / {1} / {0,1} 的子集。
    interval_set rel_eval(Op op, const interval_set& A, const interval_set& B) const {
        interval_set r;
        if (A.is_empty() || B.is_empty()) return r;
        const double amin = A.min(), amax = A.max(), bmin = B.min(), bmax = B.max();
        const double e = cfg_.eps;
        bool can_true = false, can_false = false;
        switch (op) {
            case Op::Lt:
                if (amax < bmin - e)      can_true = true;  else if (amin >= bmax - e) can_false = true; else { can_true = can_false = true; }
                break;
            case Op::Le:
                if (amax <= bmin + e)     can_true = true;  else if (amin > bmax + e)  can_false = true; else { can_true = can_false = true; }
                break;
            case Op::Gt:
                if (amin > bmax + e)      can_true = true;  else if (amax <= bmin + e) can_false = true; else { can_true = can_false = true; }
                break;
            case Op::Ge:
                if (amin >= bmax - e)     can_true = true;  else if (amax < bmin - e)  can_false = true; else { can_true = can_false = true; }
                break;
            case Op::Eq:
                if (A.max() - A.min() <= e && B.max() - B.min() <= e && std::fabs(amin - bmin) <= e) can_true = true;
                else if (amax < bmin - e || bmax < amin - e) can_false = true;
                else { can_true = can_false = true; }
                break;
            case Op::Ne:
                if (amax < bmin - e || bmax < amin - e) can_true = true;
                else if (A.max() - A.min() <= e && B.max() - B.min() <= e && std::fabs(amin - bmin) <= e) can_false = true;
                else { can_true = can_false = true; }
                break;
            default: break;
        }
        if (can_false) r.add_interval(ECFlow::interval(0, 0));
        if (can_true)  r.add_interval(ECFlow::interval(1, 1));
        return r;
    }
};

} // namespace ECFlow
