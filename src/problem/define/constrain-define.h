//------------------------Description------------------------
// ConstrainDefine:问题约束域的定义切片(原单体 Problem 的约束部分)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include "ecflow-constrain.h"
#include "ecflow-domain.h"
#include "ecflow-cardinality.h"
#include "ecflow-accumulate.h"
#include "ecflow-graph-constrain.h"
#include "variable.h"
#include "ecflow-basicfunc.h"   // stringSplit
#include "ecflow-constant.h"    // EMPTYVALUE
#include "constraint-manager.h"
#include "variable-manager.h"
#include "objective-manager.h"

namespace ECFlow
{
    enum class ConstrainKind {
        user, range, compatibility, eligible, capacity,
        unique, distinct_cap, distributed, mindistance, expression, accumulate, graph, node_sum, connectivity
    };

    // 图约束子类型（compile 时按变量长度构造对应类）：族 I 成对边 + 族 II 节点聚合(顶点 0/1)。
    enum class GraphConstrainKind { independent, distinct, clique, conflict, dominating, vertex_cover };

    class ConstrainDefine
    {
    private:
        struct Spec
        {
            ConstrainKind kind = ConstrainKind::range;
            double penalty = 1.0;
            std::vector<std::string> related_variables;
            std::vector<std::string> related_objects;
            std::vector<std::vector<double>> params;
            std::vector<long long> addresses;
            std::string formula;
            std::shared_ptr<Constrain> pre_built;   // accumulate
            GraphConstrainKind graph_kind = GraphConstrainKind::independent;  // kind==graph
            std::vector<int>   graph_edges;          // 扁平 0 基边列表 [u0,v0,...]（graph / node_sum 共用）
            bool (*graph_conflict)(double, double) = nullptr;                 // conflict 谓词
            int                ns_nv = 0;            // node_sum: 顶点数
            bool               ns_directed = false;  // node_sum: 有向(尾-1/头+1)
            std::vector<double> ns_lo, ns_hi, ns_cap;// node_sum: 每节点界 + 每边 cap
            std::vector<int>   conn_terminals;       // connectivity: 需连通的终端(空=全部)
        };
        std::vector<Spec> _specs;
        bool _no_check = false;

        static std::vector<std::string> _split(const std::string& s)
        { std::vector<std::string> v; if (!s.empty()) stringSplit(s, ',', v); return v; }

        // 图约束只存边 + 子类型(+ conflict 谓词);n 与具体类在 compile() 时由变量长度推导。
        void _pushGraphSpec(const std::string& input_var, const std::string& objs,
                            GraphConstrainKind gk, const std::vector<int>& edges,
                            bool (*conflict)(double, double), double penalty_w)
        {
            Spec d; d.kind = ConstrainKind::graph; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            d.graph_kind = gk; d.graph_edges = edges; d.graph_conflict = conflict;
            _specs.push_back(std::move(d));
        }

    public:
        void changeConstrainDeal(bool no_check) { _no_check = no_check; }

        void addConstrain(const std::string& input_elements, void (*ini)(), double (*chk)(int,double),
                          void (*chg)(int,double), double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::user; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.addresses = { reinterpret_cast<long long>(ini), reinterpret_cast<long long>(chk), reinterpret_cast<long long>(chg) };
            _specs.push_back(std::move(d));
        }
        void addConstrainRange(const std::string& input_elements, double left = EMPTYVALUE, double right = EMPTYVALUE,
                               double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::range; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.params = { { left, right } }; _specs.push_back(std::move(d));
        }

        // 该变量是否已注册过 range 约束。供 Problem::compile() 的**域进约束**查重:用户已为某变量声明 range 时
        //   不再自动补,否则两条 range 会**同时计入 constraintViolation** → 同一处越界被算两遍。
        bool hasRangeConstrain(const std::string& variable_name) const
        {
            for (const Spec& d : _specs)
                if (d.kind == ConstrainKind::range)
                    for (const std::string& v : d.related_variables)
                        if (v == variable_name) return true;
            return false;
        }
        void addConstrainCompatibility(const std::string& input_elements, const double* value, int length,
                                       double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::compatibility; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            std::vector<double> b; b.reserve(length + 1); b.push_back((double)length);
            for (int i = 0; i < length; i++) b.push_back(value[i]);
            d.params = { std::move(b) }; _specs.push_back(std::move(d));
        }
        // 逐维允许值(eligible):params[dim] = 该维允许取的值集合。FJSP=每工序的允许机器。
        void addConstrainEligible(const std::string& input_elements, std::vector<std::vector<double>> allowed,
                                  double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::eligible; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.params = std::move(allowed); _specs.push_back(std::move(d));
        }
        void addConstrainUnique(const std::string& input_elements, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::unique; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs); _specs.push_back(std::move(d));
        }
        // 至多 p 个不同值(选址层):|不同值| ≤ p,满 p 后域收缩到已现集合。
        void addConstrainDistinctCap(const std::string& input_elements, int p, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::distinct_cap; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.params = { { static_cast<double>(p) } }; _specs.push_back(std::move(d));
        }
        void addConstrainMinDistance(const std::string& input_elements, double gap_width, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::mindistance; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.params = { { 0.0, gap_width } }; _specs.push_back(std::move(d));
        }
        void addConstrainMinDistance(const std::string& input_elements, double* gap_width, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::mindistance; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            d.params = { { 1.0 } }; d.addresses = { reinterpret_cast<long long>(gap_width) }; _specs.push_back(std::move(d));
        }
        void addConstrainCapacity(const std::string& input_elements, double* caps, int nC, double* vols, int nI,
                                  double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::capacity; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            std::vector<double> a; a.reserve(nC + 1); a.push_back((double)nC); for (int i = 0; i < nC; i++) a.push_back(caps[i]);
            std::vector<double> b; b.reserve(nI + 1); b.push_back((double)nI); for (int i = 0; i < nI; i++) b.push_back(vols[i]);
            d.params = { std::move(a), std::move(b) }; _specs.push_back(std::move(d));
        }
        void addConstrainDistributed(const std::string& input_elements, double* vals, int size, int* nums,
                                     double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::distributed; d.penalty = penalty_w;
            d.related_variables = _split(input_elements); d.related_objects = _split(objs);
            std::vector<double> a; a.reserve(size + 1); a.push_back((double)size); for (int i = 0; i < size; i++) a.push_back(vals[i]);
            std::vector<double> c; c.reserve(size); for (int i = 0; i < size; i++) c.push_back((double)nums[i]);
            d.params = { std::move(a), std::move(c) }; _specs.push_back(std::move(d));
        }
        void addConstrainExpr(const std::string& input_var, const std::string& input_elements, const std::string& formula,
                              double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::expression; d.penalty = penalty_w;
            d.related_variables = _split(input_elements.empty() ? input_var : (input_var + "," + input_elements));
            d.related_objects = _split(objs); d.formula = formula; _specs.push_back(std::move(d));
        }
        void addConstrainSequenceAccumulate(const std::string& input_var, std::vector<double> delta,
                                            double lower, double upper, double init = 0.0,
                                            double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::accumulate; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            d.pre_built = std::make_shared<ConstrainSequenceAccumulate>(penalty_w, std::move(delta), lower, upper, init);
            _specs.push_back(std::move(d));
        }
        void addConstrainScheduleAccumulate(const std::string& input_var, int node_count,
                                            std::vector<std::vector<int>> pred, std::vector<std::vector<double>> exec,
                                            CommModel comm, std::vector<double> deadline,
                                            double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::accumulate; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            d.pre_built = std::make_shared<ConstrainScheduleAccumulate>(penalty_w, node_count, std::move(pred), std::move(exec), std::move(comm), std::move(deadline));
            _specs.push_back(std::move(d));
        }
        ConstrainAccumulate* addConstrainAccumulate(const std::string& input_var, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::accumulate; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            auto c = std::make_shared<ConstrainAccumulate>(penalty_w);
            ConstrainAccumulate* raw = c.get(); d.pre_built = std::move(c);
            _specs.push_back(std::move(d)); return raw;
        }

        // 族 I 图约束（成对边约束）：input_var 为顶点变量；edges 为扁平 0 基边列表
        // [u0,v0,u1,v1,...]。顶点数 n 不再显式传入——compile() 时取自变量长度，
        // 并校验所有边端点 ∈ [0,n)（见 compile / checkCompleteness）。
        void addConstrainGraphIndependent(const std::string& input_var, const std::vector<int>& edges,
                                          double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::independent, edges, nullptr, penalty_w); }

        void addConstrainGraphDistinct(const std::string& input_var, const std::vector<int>& edges,
                                       double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::distinct, edges, nullptr, penalty_w); }

        void addConstrainGraphClique(const std::string& input_var, const std::vector<int>& edges,
                                     double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::clique, edges, nullptr, penalty_w); }

        void addConstrainGraphConflict(const std::string& input_var, const std::vector<int>& edges,
                                       bool (*conflict)(double, double), double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::conflict, edges, conflict, penalty_w); }

        // 族 II（节点聚合，顶点 0/1 变量）
        void addConstrainGraphDominating(const std::string& input_var, const std::vector<int>& edges,
                                         double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::dominating, edges, nullptr, penalty_w); }

        void addConstrainGraphVertexCover(const std::string& input_var, const std::vector<int>& edges,
                                          double penalty_w = 1, const std::string& objs = "")
        { _pushGraphSpec(input_var, objs, GraphConstrainKind::vertex_cover, edges, nullptr, penalty_w); }

        // NodeSum（族 II 整合：匹配/度/流）。input_var 为**边变量**(长度=边数)；
        // edges 扁平 [tail,head]；directed=false 计数(无向)、true 有向(尾-1/头+1)；
        // lo/hi 每节点界(长度 nv)；cap 每边贡献上限(长度=边数)。
        void addConstrainNodeSum(const std::string& input_var, int n_vertices, const std::vector<int>& edges,
                                 bool directed, const std::vector<double>& lo, const std::vector<double>& hi,
                                 const std::vector<double>& cap, double penalty_w = 1, const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::node_sum; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            d.graph_edges = edges; d.ns_nv = n_vertices; d.ns_directed = directed;
            d.ns_lo = lo; d.ns_hi = hi; d.ns_cap = cap;
            _specs.push_back(std::move(d));
        }

        // Connectivity（STP / 连通子图）。input_var 为**边变量**(长度=边数)；edges 扁平边端点；
        // terminals 需连通的终端顶点(空 = 全部顶点 = 生成连通)。
        void addConstrainGraphConnectivity(const std::string& input_var, int n_vertices, const std::vector<int>& edges,
                                           const std::vector<int>& terminals = {}, double penalty_w = 1,
                                           const std::string& objs = "")
        {
            Spec d; d.kind = ConstrainKind::connectivity; d.penalty = penalty_w;
            d.related_variables = _split(input_var); d.related_objects = _split(objs);
            d.graph_edges = edges; d.ns_nv = n_vertices; d.conn_terminals = terminals;
            _specs.push_back(std::move(d));
        }

        void clear() { _specs.clear(); _no_check = false; }   // reset specs + own flags
        int getConstraintNumber() const { return static_cast<int>(_specs.size()); }

        bool checkCompleteness(const VariableManager& var, const ObjectiveManager& obj) const
        {
            for (const auto& d : _specs)
            {
                if (d.related_variables.size() != 1 && d.kind != ConstrainKind::expression) return false;
                for (const auto& vn : d.related_variables) if (var.getVariableId(vn) < 0) return false;
                for (const auto& on : d.related_objects)
                {
                    bool f = false;
                    for (int k = 0; k < obj.objectNumber(); k++) if (obj.objectiveName(k) == on) { f = true; break; }
                    if (!f) return false;
                }
            }
            return true;
        }

        // compile: definition -> runtime engine (ported from v1 ConstraintManager::buildRuntime).
        ConstraintManager compile(const VariableManager& var, const ObjectiveManager& obj) const
        {
            ConstraintManager rt;
            const int n_con = static_cast<int>(_specs.size());
            const int n_obj = obj.objectNumber();
            const int n_var = var.variable_number;
            const int n_dec = var.decision_variable_number;

            rt.variable_number = n_var; rt.decision_variable_number = n_dec; rt.object_number = n_obj;
            rt.constraint_check = !_no_check; rt.constrain_number = n_con;
            rt.constrain_pairs = new Con4ElePair[n_var];
            rt.constrains = new Constrain*[n_con];
            rt.constrain_variable_index = new int[n_con];
            rt.objective_penalty_number = new int[n_obj];
            rt.objective_penalty_index = new int*[n_obj];
            for (int i = 0; i < n_con; i++) rt.constrains[i] = nullptr;            // exception-safe
            for (int i = 0; i < n_obj; i++) rt.objective_penalty_index[i] = nullptr;

            // 1) initial feasible region from variable bounds
            rt.feasible_regions_ini.resize(n_dec); rt.feasible_regions_cur.resize(n_dec);
            for (int i = 0; i < n_dec; i++)
            {
                const ElementNote& note = var.variables[var.decision_variable_index[i]].note;
                rt.feasible_regions_ini[i] = make_domain(note._lowbound, note._upbound);
            }

            // 2) construct constraints
            for (int cid = 0; cid < n_con; cid++)
            {
                const Spec& d = _specs[cid];
                Constrain* c = nullptr;
                if (d.pre_built) c = d.pre_built->clone();
                else switch (d.kind)
                {
                case ConstrainKind::user:
                    c = new ConstrainUserDefined(d.penalty,
                        reinterpret_cast<void(*)()>(d.addresses[0]),
                        reinterpret_cast<double(*)(int,double)>(d.addresses[1]),
                        reinterpret_cast<void(*)(int,double)>(d.addresses[2])); break;
                case ConstrainKind::range:
                {
                    // 边界留空(EMPTYVALUE)→ 编译时自动填该变量声明的域。这是 addConstrainRange 的对外契约。
                    //   判定用 is_empty():原写 `left != left`(NaN 自比较惯用法)——功能正确,
                    //   但在一个满是 `== EMPTYVALUE` 恒假陷阱的代码库里,一个**长得像笔误的正确写法**本身就是负担,
                    //   且违反 代码规范.md §3「哨兵判定一律 is_empty()」。
                    double left = d.params[0][0], right = d.params[0][1];
                    if (is_empty(left) || is_empty(right))
                    {
                        int vid = var.getVariableId(d.related_variables[0]);
                        if (vid >= 0)
                        {
                            if (is_empty(left))  left  = var.variables[vid].getLowbound();
                            if (is_empty(right)) right = var.variables[vid].getUpbound();
                        }
                    }
                    c = new ConstrainRange(d.penalty, left, right); break;
                }
                case ConstrainKind::compatibility:
                    c = new ConstrainCompatibility(d.penalty, d.params[0].data() + 1, (int)d.params[0][0]); break;
                case ConstrainKind::eligible:
                    c = new ConstrainEligible(d.penalty, d.params); break;   // params[dim] = 允许值
                case ConstrainKind::capacity:
                    c = new ConstrainCapacity(d.penalty, const_cast<double*>(d.params[0].data() + 1), (int)d.params[0][0],
                                              const_cast<double*>(d.params[1].data() + 1), (int)d.params[1][0]); break;
                case ConstrainKind::unique:
                {
                    int vid = var.getVariableId(d.related_variables[0]);
                    int dvi = (vid >= 0) ? var.variable_map_index[vid] : -1;
                    double lb = (vid >= 0) ? var.variables[vid].getLowbound() : 0.0;
                    double acc = (vid >= 0) ? var.variables[vid].getAccuracy() : 1.0;
                    ViewMode vmode = (vid >= 0) ? viewModeOf(var.getVariableType(vid)) : ViewMode::grid;
                    int len = 1000;
                    if (dvi >= 0) len = domain_view(rt.feasible_regions_ini[dvi], lb, acc, vmode).count();
                    if (len < 1000)
                    {
                        int ln; double* fl = domain_view(rt.feasible_regions_ini[dvi], lb, acc, vmode).enumerate_alloc(ln);
                        c = new ConstrainUnique(d.penalty, fl, ln); delete[] fl;
                    }
                    else c = new ConstrainUniqueLarge(d.penalty);
                    break;
                }
                case ConstrainKind::distinct_cap:
                    c = new ConstrainDistinctCap(d.penalty, (int)d.params[0][0]); break;
                case ConstrainKind::distributed:
                    c = new ConstrainDistributed(d.penalty, const_cast<double*>(d.params[0].data() + 1), (int)d.params[0][0],
                                                 const_cast<double*>(d.params[1].data())); break;
                case ConstrainKind::mindistance:
                {
                    int vid = var.getVariableId(d.related_variables[0]);
                    double lo = (vid >= 0) ? var.variables[vid].getLowbound() : 0.0;
                    double hi = (vid >= 0) ? var.variables[vid].getUpbound() : 1.0;
                    int vl = (vid >= 0) ? var.variables[vid].getLength() : 1;
                    if (d.params[0][0] != 0.0) c = new ConstrainMinDistance(d.penalty, lo, hi, reinterpret_cast<double*>(d.addresses[0]), vl);
                    else c = new ConstrainMinDistance(d.penalty, lo, hi, d.params[0][1], vl);
                    break;
                }
                case ConstrainKind::expression:
                    c = new ConstrainExpression(d.penalty, d.formula, nullptr, 0, 1); break;   // formula validated here
                case ConstrainKind::graph:
                {
                    // 顶点数 n 取自所绑变量长度（不再由调用方传入，故不可能错位）。
                    int gvid = var.getVariableId(d.related_variables[0]);
                    if (gvid < 0)
                        throw std::invalid_argument("graph constraint: variable not found - " + d.related_variables[0]);
                    int n  = var.getVariableLength(gvid);
                    int ne = static_cast<int>(d.graph_edges.size()) / 2;
                    for (int e = 0; e < ne; ++e)             // 校验边端点 ∈ [0,n)
                    {
                        int u = d.graph_edges[2 * e], w = d.graph_edges[2 * e + 1];
                        if (u < 0 || u >= n || w < 0 || w >= n)
                            throw std::invalid_argument("graph constraint: edge endpoint out of range [0,"
                                                        + std::to_string(n) + ")");
                    }
                    const int* ep = d.graph_edges.data();
                    switch (d.graph_kind)
                    {
                    case GraphConstrainKind::independent: c = new ConstrainGraphIndependent(d.penalty, n, ep, ne); break;
                    case GraphConstrainKind::distinct:    c = new ConstrainGraphDistinct(d.penalty, n, ep, ne); break;
                    case GraphConstrainKind::clique:      c = new ConstrainGraphClique(d.penalty, n, ep, ne); break;
                    case GraphConstrainKind::conflict:    c = new ConstrainGraphConflict(d.penalty, n, ep, ne, d.graph_conflict); break;
                    case GraphConstrainKind::dominating:  c = new ConstrainGraphDominating(d.penalty, n, ep, ne); break;
                    case GraphConstrainKind::vertex_cover:c = new ConstrainGraphVertexCover(d.penalty, n, ep, ne); break;
                    }
                    break;
                }
                case ConstrainKind::node_sum:
                {
                    int gvid = var.getVariableId(d.related_variables[0]);
                    if (gvid < 0)
                        throw std::invalid_argument("node_sum: variable not found - " + d.related_variables[0]);
                    int ne_var = var.getVariableLength(gvid);              // |E| 取自边变量长度
                    int ne     = static_cast<int>(d.graph_edges.size()) / 2;
                    if (ne != ne_var)
                        throw std::invalid_argument("node_sum: edge count != variable length");
                    if ((int)d.ns_lo.size() != d.ns_nv || (int)d.ns_hi.size() != d.ns_nv)
                        throw std::invalid_argument("node_sum: lo/hi size != n_vertices");
                    if ((int)d.ns_cap.size() != ne)
                        throw std::invalid_argument("node_sum: cap size != edge count");
                    for (int e = 0; e < ne; ++e)                           // 端点 ∈ [0, nv)
                    {
                        int t = d.graph_edges[2 * e], h = d.graph_edges[2 * e + 1];
                        if (t < 0 || t >= d.ns_nv || h < 0 || h >= d.ns_nv)
                            throw std::invalid_argument("node_sum: edge endpoint out of range [0,"
                                                        + std::to_string(d.ns_nv) + ")");
                    }
                    c = new ConstrainNodeSum(d.penalty, d.ns_nv, ne, d.graph_edges.data(), d.ns_directed,
                                             d.ns_lo, d.ns_hi, d.ns_cap);
                    break;
                }
                case ConstrainKind::connectivity:
                {
                    int gvid = var.getVariableId(d.related_variables[0]);
                    if (gvid < 0)
                        throw std::invalid_argument("connectivity: variable not found - " + d.related_variables[0]);
                    int ne = static_cast<int>(d.graph_edges.size()) / 2;
                    if (ne != var.getVariableLength(gvid))
                        throw std::invalid_argument("connectivity: edge count != variable length");
                    for (int e = 0; e < ne; ++e)
                    {
                        int t = d.graph_edges[2 * e], h = d.graph_edges[2 * e + 1];
                        if (t < 0 || t >= d.ns_nv || h < 0 || h >= d.ns_nv)
                            throw std::invalid_argument("connectivity: edge endpoint out of range");
                    }
                    for (int t : d.conn_terminals)
                        if (t < 0 || t >= d.ns_nv) throw std::invalid_argument("connectivity: terminal out of range");
                    c = new ConstrainGraphConnectivity(d.penalty, d.ns_nv, ne, d.graph_edges.data(), d.conn_terminals);
                    break;
                }
                default: c = nullptr; break;
                }

                rt.constrains[cid] = c;
                for (const auto& vn : d.related_variables)
                {
                    int vid = var.getVariableId(vn); if (vid < 0) continue;
                    rt.constrain_pairs[vid].constrains.push_back(c);
                    rt.constrain_variable_index[cid] = vid;
                    if (c && c->getConstrainLevel() == constraints_range)
                    {
                        int dvi = var.variable_map_index[vid];
                        if (dvi >= 0)
                        {
                            domain_view dv(rt.feasible_regions_ini[dvi], var.variables[vid].getLowbound(), var.variables[vid].getAccuracy(),
                                           viewModeOf(var.getVariableType(vid)));
                            c->regionReduction(0, dv);
                        }
                    }
                    break;
                }
            }

            // 3) no-dem-reduction table
            rt.no_dem_reduction_region = new bool[n_dec];
            for (int vid = 0; vid < n_dec; vid++)
            {
                rt.no_dem_reduction_region[vid] = true;
                int real_vid = var.decision_variable_index[vid];
                for (int i = 0; i < (int)rt.constrain_pairs[real_vid].constrains.size(); i++)
                    if (rt.constrain_pairs[real_vid].constrains[i]->getConstrainLevel() == constraints_discrete)
                    { rt.no_dem_reduction_region[vid] = false; break; }
            }

            // 4) objective penalty tables
            std::vector<int> pl;
            for (int oid = 0; oid < n_obj; oid++)
            {
                pl.clear();
                for (int cid = 0; cid < n_con; cid++)
                    for (const auto& on : _specs[cid].related_objects)
                        if (obj.objectiveName(oid) == on) pl.push_back(cid);
                rt.objective_penalty_number[oid] = (int)pl.size();
                rt.objective_penalty_index[oid] = new int[pl.size() ? pl.size() : 1];
                for (int i = 0; i < (int)pl.size(); i++) rt.objective_penalty_index[oid][i] = pl[i];
            }

            // 5) cur = ini
            for (int i = 0; i < n_dec; i++) rt.feasible_regions_cur[i] = rt.feasible_regions_ini[i];
            return rt;
        }
    };
}
