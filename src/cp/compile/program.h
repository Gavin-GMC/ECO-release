#pragma once
//
// cp/compile/program.h
//
// 编译产物 Program：把表达式约束系统编译成「扁平节点数组 + 符号表 + 约束根表」。
// 不可变、只读、可跨线程共享；不持有任何变量值或中间缓存（那些在 State 里）。
//
// 变量支持标量与向量：
//  - 标量 x：占 1 个槽位。
//  - 向量 a[i]（常量下标）：同名向量占一段【连续】槽位 [first, first+length)，
//    length = 最大下标+1，按下标顺序排布。便于外部 vector 按连续区间映射到槽位。
//  - 同名不可同时作标量与向量。槽位整体顺序 = 基名首次出现序；向量内部按下标序。
//
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

namespace ECFlow {

using NodeId = std::int32_t;
constexpr NodeId kNoNode = -1;

enum class Op : std::uint8_t {
    Const, Var,
    Add, Sub, Mul, Div, Neg,          // 算术
    Lt, Le, Gt, Ge, Eq, Ne            // 关系（结果域 ⊆ {0,1}）
};

inline bool is_relational(Op op) {
    switch (op) {
        case Op::Lt: case Op::Le: case Op::Gt:
        case Op::Ge: case Op::Eq: case Op::Ne: return true;
        default: return false;
    }
}
inline bool is_binary(Op op) {
    switch (op) {
        case Op::Add: case Op::Sub: case Op::Mul: case Op::Div:
        case Op::Lt: case Op::Le: case Op::Gt:
        case Op::Ge: case Op::Eq: case Op::Ne: return true;
        default: return false;
    }
}

struct Node {
    Op      op;
    NodeId  a    = kNoNode;
    NodeId  b    = kNoNode;
    int32_t slot = -1;        // Var：编译后为最终槽位；编译中为临时符号 id
    double  value = 0.0;
};

struct Constraint {
    NodeId  root;
    double  weight = 1.0;
};

class parse_error : public std::runtime_error {
public:
    explicit parse_error(const std::string& msg) : std::runtime_error(msg) {}
};

// 编译期发射器。变量按 (base,index) 收集，index<0 表标量；最终槽位由 Program 布局。
class Emitter {
public:
    struct VarKey { std::string base; int index; };

    std::vector<Node>       nodes;
    std::vector<Constraint> constraints;
    std::vector<VarKey>     var_refs;     // 临时符号 id -> (base,index)

    NodeId var(const std::string& base, int index = -1) {
        std::string key = index < 0 ? base : base + "\x01" + std::to_string(index);
        auto it = key_to_node_.find(key);
        if (it != key_to_node_.end()) return it->second;
        int32_t pid = static_cast<int32_t>(var_refs.size());
        var_refs.push_back(VarKey{base, index});
        NodeId id = push(Node{Op::Var, kNoNode, kNoNode, pid, 0.0});
        key_to_node_.emplace(key, id);
        return id;
    }
    NodeId konst(double v)                 { return push(Node{Op::Const, kNoNode, kNoNode, -1, v}); }
    NodeId unary(Op op, NodeId a)          { return push(Node{op, a, kNoNode, -1, 0.0}); }
    NodeId binary(Op op, NodeId a, NodeId b){ return push(Node{op, a, b, -1, 0.0}); }
    void add_constraint(NodeId root, double weight) { constraints.push_back(Constraint{root, weight}); }

private:
    std::unordered_map<std::string, NodeId> key_to_node_;
    NodeId push(const Node& n) { nodes.push_back(n); return static_cast<NodeId>(nodes.size() - 1); }
};

class Program {
public:
    struct VarInfo { int32_t first_slot; int32_t length; bool is_vector; };

    Program() = default;

    static Program from_emitter(Emitter&& e) {
        Program p;
        p.nodes_       = std::move(e.nodes);
        p.constraints_ = std::move(e.constraints);
        p.layout_variables(e.var_refs);
        p.build_var_index();
        return p;
    }

    std::size_t node_count()     const { return nodes_.size(); }
    std::size_t variable_count() const { return slot_names_.size(); }

    const std::vector<Node>&        nodes()          const { return nodes_; }
    const std::vector<Constraint>&  constraints()    const { return constraints_; }
    const std::vector<std::string>& variable_names() const { return slot_names_; }
    const Node& node(NodeId id) const { return nodes_[static_cast<std::size_t>(id)]; }

    // 变量名（裸名 "a" 即 a[0]）或形如 "a[2]" 的字符串 -> 槽位；不存在返回 -1。
    int32_t slot(const std::string& name) const {
        auto br = name.find('[');
        if (br == std::string::npos) {
            auto it = bases_.find(name);
            return it != bases_.end() ? it->second.first_slot : -1;   // 裸名 -> 分量 0
        }
        auto rb = name.find(']', br);
        if (rb == std::string::npos) return -1;
        std::string base = name.substr(0, br);
        int index = 0;
        try { index = std::stoi(name.substr(br + 1, rb - br - 1)); } catch (...) { return -1; }
        return slot(base, index);
    }
    // 分量 (base, index) -> 槽位；越界/不存在返回 -1。一切皆向量，长度≥1，分量 0 总有效。
    int32_t slot(const std::string& base, int index) const {
        auto it = bases_.find(base);
        if (it == bases_.end()) return -1;
        if (index < 0 || index >= it->second.length) return -1;
        return it->second.first_slot + index;
    }

    bool    is_vector(const std::string& base) const {
        auto it = bases_.find(base); return it != bases_.end() && it->second.is_vector;
    }
    // 向量长度；标量返回 1；未知返回 0。
    int32_t vector_length(const std::string& base) const {
        auto it = bases_.find(base);
        return it == bases_.end() ? 0 : it->second.length;
    }
    const std::string& variable_name(int32_t slot) const { return slot_names_[static_cast<std::size_t>(slot)]; }

    const std::vector<int>& constraints_of(int32_t slot) const { return var_to_cons_[slot]; }

private:
    std::vector<Node>        nodes_;
    std::vector<Constraint>  constraints_;
    std::vector<std::string> slot_names_;                 // 槽位 -> 展示名
    std::unordered_map<std::string, VarInfo> bases_;      // 基名 -> 布局信息
    std::vector<std::vector<int>> var_to_cons_;

    void layout_variables(const std::vector<Emitter::VarKey>& refs) {
        // 一切皆向量：裸名 a 即 a[0]。每个基名长度 = 最大下标+1，分量数>1 才算「向量」。
        std::vector<std::string> base_order;
        std::unordered_map<std::string, int> maxidx;
        for (const auto& k : refs) {
            auto it = maxidx.find(k.base);
            if (it == maxidx.end()) { maxidx.emplace(k.base, k.index); base_order.push_back(k.base); }
            else it->second = std::max(it->second, k.index);
        }
        int32_t cursor = 0;
        for (const auto& base : base_order) {
            int32_t len = maxidx[base] + 1;
            bool vec = len > 1;
            bases_[base] = VarInfo{cursor, len, vec};
            for (int32_t i = 0; i < len; ++i)
                slot_names_.push_back(vec ? base + "[" + std::to_string(i) + "]" : base);
            cursor += len;
        }
        // 把 Var 节点的临时符号 id 重映射为最终槽位
        for (auto& n : nodes_) {
            if (n.op != Op::Var) continue;
            const Emitter::VarKey& k = refs[static_cast<std::size_t>(n.slot)];
            n.slot = bases_[k.base].first_slot + k.index;
        }
    }

    void build_var_index() {
        var_to_cons_.assign(slot_names_.size(), {});
        std::vector<char> seen(nodes_.size(), 0);
        for (int c = 0; c < static_cast<int>(constraints_.size()); ++c) {
            std::fill(seen.begin(), seen.end(), (char)0);
            collect_vars(constraints_[c].root, seen, c);
        }
    }
    void collect_vars(NodeId id, std::vector<char>& seen, int c) {
        if (id < 0 || seen[id]) return;
        seen[id] = 1;
        const Node& n = nodes_[id];
        if (n.op == Op::Var) {
            auto& v = var_to_cons_[n.slot];
            if (v.empty() || v.back() != c) v.push_back(c);
            return;
        }
        if (n.a >= 0) collect_vars(n.a, seen, c);
        if (n.b >= 0) collect_vars(n.b, seen, c);
    }
};

} // namespace ECFlow
