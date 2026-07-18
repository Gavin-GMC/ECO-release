//------------------------Description------------------------
// 图约束族 I「成对边约束」:对每条边在两端点取值上施加关系(Independent/Distinct/Clique/Conflict)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <algorithm>
#include "ecflow-constrain.h"   // Constrain, ConstrianLevel, domain_view, EMPTYVALUE

namespace ECFlow
{
    // ---------------------------------------------------------------
    // 纯工具：CSR 邻接（无向、双向存）。由各图约束**组合**持有，非继承。
    // ---------------------------------------------------------------
    struct GraphAdjacency
    {
        int              n = 0;
        std::vector<int> off;   // [n+1]
        std::vector<int> adj;   // [2*E]

        void build(int n_, const int* edges, int num_edges)
        {
            n = n_;
            std::vector<int> deg(n, 0);
            for (int e = 0; e < num_edges; ++e) { ++deg[edges[2 * e]]; ++deg[edges[2 * e + 1]]; }
            off.assign(n + 1, 0);
            for (int i = 0; i < n; ++i) off[i + 1] = off[i] + deg[i];
            adj.assign(off[n], 0);
            std::vector<int> cur(off.begin(), off.begin() + n);
            for (int e = 0; e < num_edges; ++e)
            {
                int u = edges[2 * e], v = edges[2 * e + 1];
                adj[cur[u]++] = v;
                adj[cur[v]++] = u;
            }
        }
        int  begin(int v) const { return off[v]; }
        int  end(int v)   const { return off[v + 1]; }
        bool adjacent(int u, int v) const            // u 是否为 v 的邻居
        { for (int k = off[v]; k < off[v + 1]; ++k) if (adj[k] == u) return true; return false; }
    };

    // 顶点是否**已决定**(逐维构造中尚未定值的顶点其 _value 为 EMPTYVALUE)。
    //   (另两个:is_empty / 已删的 is_nan),且**反向 + 私有 + 又一个名字**。归并到唯一入口 is_empty():
    //   `_decided(v)` ⟺ `!is_empty(v)`。保留本薄封装仅为可读性(6 处调用读作"已决定"比"非空"贴合图约束语境)。
    static inline bool _decided(double v) { return !is_empty(v); }

    // ---------------------------------------------------------------
    // Independent（MIS）：相邻不同时选；禁止取值对 {(1,1)}。
    // ---------------------------------------------------------------
    class ConstrainGraphIndependent final : public Constrain
    {
        GraphAdjacency    _g;
        std::vector<char> _selected;   // 运行态：顶点已选(value>=0.5)
    public:
        ConstrainGraphIndependent(double w, int n, const int* edges, int num_edges)
            : Constrain(w) { _g.build(n, edges, num_edges); _selected.assign(n, 0); }

        void ini() override { std::fill(_selected.begin(), _selected.end(), (char)0); }
        void update(int v, double value) override { if (v >= 0 && v < _g.n) _selected[v] = (value >= 0.5) ? 1 : 0; }
        bool meet(int v, double value) override
        {
            if (value < 0.5) return true;
            for (int k = _g.begin(v); k < _g.end(v); ++k) if (_selected[_g.adj[k]]) return false;
            return true;
        }
        void regionReduction(int v, domain_view& region) override
        {
            for (int k = _g.begin(v); k < _g.end(v); ++k)
                if (_selected[_g.adj[k]]) { region.remove_point(1.0); return; }
        }
        double violation(double* x, int n) override
        {
            int c = 0, m = (n < _g.n) ? n : _g.n;
            for (int u = 0; u < m; ++u) if (x[u] >= 0.5)
                for (int k = _g.begin(u); k < _g.end(u); ++k)
                { int w = _g.adj[k]; if (w > u && w < m && x[w] >= 0.5) ++c; }
            return c;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainGraphIndependent(*this); }
    };

    // ---------------------------------------------------------------
    // Distinct：相邻取值必异；禁止取值对 {(c,c) ∀c}（相邻同值即违反）。
    //   注意：与 Independent 不同——这里 (0,0) 也违反（相邻同值）。
    // ---------------------------------------------------------------
    class ConstrainGraphDistinct final : public Constrain
    {
        GraphAdjacency      _g;
        std::vector<double> _value;    // 运行态：顶点取值(EMPTYVALUE=未决定)
    public:
        ConstrainGraphDistinct(double w, int n, const int* edges, int num_edges)
            : Constrain(w) { _g.build(n, edges, num_edges); _value.assign(n, EMPTYVALUE); }

        void ini() override { std::fill(_value.begin(), _value.end(), EMPTYVALUE); }
        void update(int v, double value) override { if (v >= 0 && v < _g.n) _value[v] = value; }
        bool meet(int v, double value) override
        {
            for (int k = _g.begin(v); k < _g.end(v); ++k)
            { double nb = _value[_g.adj[k]]; if (_decided(nb) && nb == value) return false; }
            return true;
        }
        void regionReduction(int v, domain_view& region) override
        {
            for (int k = _g.begin(v); k < _g.end(v); ++k)
            { double nb = _value[_g.adj[k]]; if (_decided(nb)) region.remove_point(nb); }
        }
        double violation(double* x, int n) override
        {
            int c = 0, m = (n < _g.n) ? n : _g.n;
            for (int u = 0; u < m; ++u)
                for (int k = _g.begin(u); k < _g.end(u); ++k)
                { int w = _g.adj[k]; if (w > u && w < m && x[u] == x[w]) ++c; }
            return c;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainGraphDistinct(*this); }
    };

    // ---------------------------------------------------------------
    // Clique：选中集两两相邻（= 补图上的独立性）。
    //   v 可取 1，当且仅当 v 与**所有已选顶点**相邻。
    // ---------------------------------------------------------------
    class ConstrainGraphClique final : public Constrain
    {
        GraphAdjacency   _g;
        std::vector<int> _sel;   // 运行态：已选顶点列表
    public:
        ConstrainGraphClique(double w, int n, const int* edges, int num_edges)
            : Constrain(w) { _g.build(n, edges, num_edges); }

        void ini() override { _sel.clear(); }
        void update(int v, double value) override { if (value >= 0.5) _sel.push_back(v); }
        bool meet(int v, double value) override
        {
            if (value < 0.5) return true;
            for (int u : _sel) if (u != v && !_g.adjacent(u, v)) return false;
            return true;
        }
        void regionReduction(int v, domain_view& region) override
        {
            for (int u : _sel) if (u != v && !_g.adjacent(u, v)) { region.remove_point(1.0); return; }
        }
        double violation(double* x, int n) override
        {
            int m = (n < _g.n) ? n : _g.n, c = 0;
            std::vector<int> s;
            for (int i = 0; i < m; ++i) if (x[i] >= 0.5) s.push_back(i);
            for (size_t i = 0; i < s.size(); ++i)
                for (size_t j = i + 1; j < s.size(); ++j)
                    if (!_g.adjacent(s[i], s[j])) ++c;   // 选中却非相邻的对
            return c;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainGraphClique(*this); }
    };

    // ---------------------------------------------------------------
    // Conflict（通用）：每边给定冲突谓词 conflict(a,b)——(x[u],x[v]) 命中即违反。
    //   Independent = conflict(a,b)=(a>=0.5&&b>=0.5)；Distinct = conflict(a,b)=(a==b)。
    //   通用性代价：regionReduction 需枚举域逐值判定（适合小域）。
    // ---------------------------------------------------------------
    class ConstrainGraphConflict final : public Constrain
    {
        GraphAdjacency      _g;
        std::vector<double> _value;
        bool (*_conflict)(double, double) = nullptr;
    public:
        ConstrainGraphConflict(double w, int n, const int* edges, int num_edges, bool (*conflict)(double, double))
            : Constrain(w), _conflict(conflict) { _g.build(n, edges, num_edges); _value.assign(n, EMPTYVALUE); }

        void ini() override { std::fill(_value.begin(), _value.end(), EMPTYVALUE); }
        void update(int v, double value) override { if (v >= 0 && v < _g.n) _value[v] = value; }
        bool meet(int v, double value) override
        {
            for (int k = _g.begin(v); k < _g.end(v); ++k)
            { double nb = _value[_g.adj[k]]; if (_decided(nb) && _conflict(value, nb)) return false; }
            return true;
        }
        void regionReduction(int v, domain_view& region) override
        {
            std::vector<double> pts = region.enumerate();          // 通用：逐值判定
            for (double w : pts)
                for (int k = _g.begin(v); k < _g.end(v); ++k)
                { double nb = _value[_g.adj[k]]; if (_decided(nb) && _conflict(w, nb)) { region.remove_point(w); break; } }
        }
        double violation(double* x, int n) override
        {
            int c = 0, m = (n < _g.n) ? n : _g.n;
            for (int u = 0; u < m; ++u)
                for (int k = _g.begin(u); k < _g.end(u); ++k)
                { int w = _g.adj[k]; if (w > u && w < m && _conflict(x[u], x[w])) ++c; }
            return c;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainGraphConflict(*this); }
    };

    // ---------------------------------------------------------------
    // 族 II「节点聚合约束」（顶点 0/1 变量）。
    // ---------------------------------------------------------------

    // Dominating（MDS）：每个顶点被支配——v 入集 或 某邻居入集。
    //   单点选择不会"立即非法"（可能被后续邻居支配）→ meet 恒真、violation 兜底惩罚。
    //   构造期缩减（基于闭邻域未决成员的"最后一根稻草"）：对每个覆盖目标 v 维护
    //   闭邻域 N[v]={v}∪邻居 中"未决定成员数 _remaining[v]" 与"是否已有人入集 _satisfied[v]"。
    //   决定 w 时（w 尚未提交，仍计入 _remaining），若存在目标 v∈N[w] 满足
    //   未满足 且 _remaining[v]==1（w 是 N[v] 最后未决成员），则 w 取 0 会令 v 必然失配
    //   → 强制 w=1（删去取值 0）。该缩减对零违反解 sound（删的都是必非法的扩展）。
    //   violation = 未被支配的顶点数（缩减完整施加时构造期恒 0，惩罚成兜底）。
    class ConstrainGraphDominating final : public Constrain
    {
        GraphAdjacency    _g;
        std::vector<int>  _remaining;   // 运行态：N[v] 未决定成员数，init = deg(v)+1
        std::vector<char> _satisfied;   // 运行态：N[v] 是否已有入集
    public:
        ConstrainGraphDominating(double w, int n, const int* edges, int num_edges)
            : Constrain(w) { _g.build(n, edges, num_edges); _remaining.assign(n, 0); _satisfied.assign(n, 0); }

        void ini() override
        {
            for (int v = 0; v < _g.n; ++v) { _remaining[v] = (_g.end(v) - _g.begin(v)) + 1; _satisfied[v] = 0; }
        }
        void update(int w, double value) override
        {
            if (w < 0 || w >= _g.n) return;
            bool sel = (value >= 0.5);
            if (_remaining[w] > 0) --_remaining[w];           // 目标 = N[w] = {w} ∪ 邻居
            if (sel) _satisfied[w] = 1;
            for (int k = _g.begin(w); k < _g.end(w); ++k)
            {
                int v = _g.adj[k];
                if (_remaining[v] > 0) --_remaining[v];
                if (sel) _satisfied[v] = 1;
            }
        }
        bool meet(int, double) override { return true; }      // 单点永不"立即非法"
        void regionReduction(int w, domain_view& region) override
        {
            if (w < 0 || w >= _g.n) return;
            if (!_satisfied[w] && _remaining[w] == 1) { region.remove_point(0.0); return; }   // 自身目标
            for (int k = _g.begin(w); k < _g.end(w); ++k)
            { int v = _g.adj[k]; if (!_satisfied[v] && _remaining[v] == 1) { region.remove_point(0.0); return; } }
        }
        double violation(double* x, int n) override
        {
            int m = (n < _g.n) ? n : _g.n, undom = 0;
            for (int v = 0; v < m; ++v)
            {
                if (x[v] >= 0.5) continue;                       // 自身入集
                bool dom = false;
                for (int k = _g.begin(v); k < _g.end(v); ++k)
                { int u = _g.adj[k]; if (u < m && x[u] >= 0.5) { dom = true; break; } }
                if (!dom) ++undom;
            }
            return undom;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }   // 参与构造期缩减
        Constrain* clone() override { return new ConstrainGraphDominating(*this); }
    };

    // VertexCover：每条边至少一端入集（x[u]=1 或 x[v]=1）。
    //   与 Independent 对偶：邻居"已定为 0" → 本点必为 1（否则该边没覆盖）→ 删去值 0。
    //   violation = 两端都为 0 的边数。
    class ConstrainGraphVertexCover final : public Constrain
    {
        GraphAdjacency      _g;
        std::vector<double> _value;   // 运行态：顶点取值(EMPTYVALUE=未决定)
    public:
        ConstrainGraphVertexCover(double w, int n, const int* edges, int num_edges)
            : Constrain(w) { _g.build(n, edges, num_edges); _value.assign(n, EMPTYVALUE); }

        void ini() override { std::fill(_value.begin(), _value.end(), EMPTYVALUE); }
        void update(int v, double value) override { if (v >= 0 && v < _g.n) _value[v] = value; }
        bool meet(int v, double value) override
        {
            if (value >= 0.5) return true;                       // 选 1 → 覆盖其全部边
            for (int k = _g.begin(v); k < _g.end(v); ++k)
            { double nb = _value[_g.adj[k]]; if (_decided(nb) && nb < 0.5) return false; }   // 邻居已为0 → 该边未覆盖
            return true;
        }
        void regionReduction(int v, domain_view& region) override
        {
            for (int k = _g.begin(v); k < _g.end(v); ++k)
            { double nb = _value[_g.adj[k]]; if (_decided(nb) && nb < 0.5) { region.remove_point(0.0); return; } }
        }
        double violation(double* x, int n) override
        {
            int m = (n < _g.n) ? n : _g.n, c = 0;
            for (int u = 0; u < m; ++u)
                for (int k = _g.begin(u); k < _g.end(u); ++k)
                { int w = _g.adj[k]; if (w > u && w < m && x[u] < 0.5 && x[w] < 0.5) ++c; }
            return c;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainGraphVertexCover(*this); }
    };

    // ---------------------------------------------------------------
    // NodeSum：节点聚合的统一约束（变量按**边**索引 y[e]）。
    //   每节点 v:  lo_v ≤ Σ_{e∋v} coef(v,e)·y[e] ≤ hi_v
    //   coef：无向(directed=false)两端 +1；有向(true)尾 −1 / 头 +1。
    //   覆盖 匹配([0,1],count) / 度([lo,hi],count) / 流量守恒([0,0],signed)。
    //   构造期缩减：每节点线性区间反向传播（Tier 1：rest 用静态 cap）。见 docs/图约束-NodeSum设计.md。
    // ---------------------------------------------------------------
    class ConstrainNodeSum final : public Constrain
    {
        int                 _nv = 0, _ne = 0;
        std::vector<int>    _etail, _ehead;     // [ne] 每边两端点(无向时仅作两端,coef 均 +1)
        bool                _directed = false;
        std::vector<double> _lo, _hi;           // [nv] 每节点界
        std::vector<double> _cap;               // [ne] 每边贡献上限(count=1, flow=容量)
        // 运行态
        std::vector<double> _fixed;             // [nv] 已定边贡献和
        std::vector<double> _rpos, _rneg;       // [nv] 未定边正/负系数贡献和(±cap)

        double ctail() const { return _directed ? -1.0 : 1.0; }
        double chead() const { return 1.0; }

    public:
        ConstrainNodeSum(double w, int nv, int ne, const int* edges, bool directed,
                         const std::vector<double>& lo, const std::vector<double>& hi,
                         const std::vector<double>& cap)
            : Constrain(w), _nv(nv), _ne(ne), _directed(directed), _lo(lo), _hi(hi), _cap(cap)
        {
            _etail.resize(ne); _ehead.resize(ne);
            for (int e = 0; e < ne; ++e) { _etail[e] = edges[2 * e]; _ehead[e] = edges[2 * e + 1]; }
            _fixed.assign(nv, 0.0); _rpos.assign(nv, 0.0); _rneg.assign(nv, 0.0);
            ini();
        }

        void ini() override
        {
            std::fill(_fixed.begin(), _fixed.end(), 0.0);
            std::fill(_rpos.begin(), _rpos.end(), 0.0);
            std::fill(_rneg.begin(), _rneg.end(), 0.0);
            for (int e = 0; e < _ne; ++e)
            {
                _addRest(_etail[e], ctail(), _cap[e]);
                _addRest(_ehead[e], chead(), _cap[e]);
            }
        }
        void update(int e, double val) override
        {
            if (e < 0 || e >= _ne) return;
            _fixed[_etail[e]] += ctail() * val;  _delRest(_etail[e], ctail(), _cap[e]);
            _fixed[_ehead[e]] += chead() * val;  _delRest(_ehead[e], chead(), _cap[e]);
        }
        bool meet(int e, double val) override
        {
            return _feasibleEnd(_etail[e], ctail(), e, val) && _feasibleEnd(_ehead[e], chead(), e, val);
        }
        void regionReduction(int e, domain_view& region) override
        {
            double A = -1e300, B = 1e300;
            _boundEnd(_etail[e], ctail(), e, A, B);
            _boundEnd(_ehead[e], chead(), e, A, B);
            region.restrict(A, B);
        }
        double violation(double* y, int m) override
        {
            int ne = (m < _ne) ? m : _ne;
            std::vector<double> S(_nv, 0.0);
            for (int e = 0; e < ne; ++e) { S[_etail[e]] += ctail() * y[e]; S[_ehead[e]] += chead() * y[e]; }
            double v = 0;
            for (int u = 0; u < _nv; ++u)
            {
                if (S[u] < _lo[u]) v += _lo[u] - S[u];
                else if (S[u] > _hi[u]) v += S[u] - _hi[u];
            }
            return v;
        }
        ConstrianLevel getConstrainLevel() override { return constraints_discrete; }
        Constrain* clone() override { return new ConstrainNodeSum(*this); }

    private:
        // (NodeSum helpers below)
        void _addRest(int v, double c, double cap) { if (c > 0) _rpos[v] += c * cap; else _rneg[v] += c * cap; }
        void _delRest(int v, double c, double cap) { if (c > 0) _rpos[v] -= c * cap; else _rneg[v] -= c * cap; }

        // 排除 e0 后,本端点其余未定边的最大/最小贡献。
        void _restExcl(int v, double c0, int e0, double& restHi, double& restLo) const
        {
            restHi = _rpos[v] - (c0 > 0 ? c0 * _cap[e0] : 0.0);
            restLo = _rneg[v] - (c0 < 0 ? c0 * _cap[e0] : 0.0);
        }
        bool _feasibleEnd(int v, double c0, int e0, double val) const
        {
            double restHi, restLo; _restExcl(v, c0, e0, restHi, restLo);
            double base = _fixed[v] + c0 * val;
            const double eps = 1e-9;
            if (base + restLo > _hi[v] + eps) return false;   // 连最小都超 hi
            if (base + restHi < _lo[v] - eps) return false;   // 连最大都够不到 lo
            return true;
        }
        void _boundEnd(int v, double c0, int e0, double& A, double& B) const
        {
            double restHi, restLo; _restExcl(v, c0, e0, restHi, restLo);
            double loC = _lo[v] - _fixed[v] - restHi;     // c0·y[e0] 下界
            double hiC = _hi[v] - _fixed[v] - restLo;     // c0·y[e0] 上界
            double a, b;
            if (c0 > 0) { a = loC; b = hiC; } else { a = -hiC; b = -loC; }  // 除以 c0(±1)
            if (a > A) A = a;
            if (b < B) B = b;
        }
    };

    // ---------------------------------------------------------------
    // 族 III · Connectivity（STP / 连通子图）。变量按**边** 0/1。
    //   规则：选中边须把给定**终端集**连成一个连通分量（终端集空 = 全部顶点 = 生成连通）。
    //   全局约束、不可逐边分解 → meet 恒真、不缩减、**纯惩罚**：
    //     violation = 终端落入的连通分量数 − 1（0 表示全连通）。
    //   无环/树、路径：留作未来 TODO；回路由"排列 + unique"覆盖，不在此实现。
    // ---------------------------------------------------------------
    class ConstrainGraphConnectivity final : public Constrain
    {
        int              _nv = 0, _ne = 0;
        std::vector<int> _etail, _ehead;
        std::vector<int> _terminals;        // 需连通的顶点（空 → 全部）
    public:
        ConstrainGraphConnectivity(double w, int nv, int ne, const int* edges, const std::vector<int>& terminals)
            : Constrain(w), _nv(nv), _ne(ne), _terminals(terminals)
        {
            _etail.resize(ne); _ehead.resize(ne);
            for (int e = 0; e < ne; ++e) { _etail[e] = edges[2 * e]; _ehead[e] = edges[2 * e + 1]; }
            if (_terminals.empty()) { _terminals.resize(nv); for (int i = 0; i < nv; ++i) _terminals[i] = i; }
        }

        void ini() override {}
        void update(int, double) override {}
        bool meet(int, double) override { return true; }
        void regionReduction(int, domain_view&) override {}

        double violation(double* y, int m) override
        {
            int ne = (m < _ne) ? m : _ne;
            std::vector<int> par(_nv);
            for (int i = 0; i < _nv; ++i) par[i] = i;
            for (int e = 0; e < ne; ++e) if (y[e] >= 0.5)
            {
                int a = _etail[e]; while (par[a] != a) { par[a] = par[par[a]]; a = par[a]; }
                int b = _ehead[e]; while (par[b] != b) { par[b] = par[par[b]]; b = par[b]; }
                if (a != b) par[a] = b;
            }
            std::vector<int> roots; roots.reserve(_terminals.size());
            for (int t : _terminals) { int r = t; while (par[r] != r) { par[r] = par[par[r]]; r = par[r]; } roots.push_back(r); }
            std::sort(roots.begin(), roots.end());
            roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
            return roots.empty() ? 0.0 : static_cast<double>(roots.size() - 1);
        }
        ConstrianLevel getConstrainLevel() override { return constrains_non; }   // 纯惩罚
        Constrain* clone() override { return new ConstrainGraphConnectivity(*this); }
    };
}
