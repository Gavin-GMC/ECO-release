//------------------------Description------------------------
// 最小支配集 (Minimum Dominating Set) 问题模板。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------
#pragma once
#include "pt-instance-path.h"   // v1.4.9:load 路径自适应(短名/完整路径)
#include "problem.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "logger.hpp"

namespace ECFlow
{
    class PT_MDS
    {
    private:
        std::string      _name = "mds";
        int              _n = 0;
        std::vector<int> _edges;
        double           _penalty = 0;        // 0 → 默认 1e6
        std::vector<int> _perm;               // 新序 i → 原顶点

        struct sizeFunc : eccalcul_functor
        {
            int n;
            explicit sizeFunc(int n_) : n(n_) {}
            double operator()(double** a) const { double s = 0; for (int i = 0; i < n; ++i) s += a[0][i]; return s; }
            eccalcul_functor* copy() { return new sizeFunc(n); }
        };
        // 覆盖感知启发式：选 1 当且仅当本顶点尚未被已决定邻居支配。
        struct coverFunc : eccalcul_functor
        {
            std::vector<int> off, adj;        // CSR 邻接(重标号空间)
            coverFunc(std::vector<int> o, std::vector<int> a) : off(std::move(o)), adj(std::move(a)) {}
            double operator()(double** in) const
            {
                int v = static_cast<int>(in[0][0] + 0.5);
                if (in[1][0] < 0.5) return 0.5;               // 不选的基准分
                const double* x = in[2];                      // 部分解(x[<v] 已定)
                for (int k = off[v]; k < off[v + 1]; ++k) { int w = adj[k]; if (w < v && x[w] >= 0.5) return 0.0; }  // 已被邻居支配 → 跳
                return 1.0;                                   // 未支配 → 选
            }
            eccalcul_functor* copy() { return new coverFunc(*this); }
        };

    public:
        PT_MDS() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _n; }
        void setPenalty(double p) { _penalty = p; }
        const std::vector<int>& getPermutation() const { return _perm; }

        void setGraph(int n, const std::vector<int>& edges) { _n = n; _edges = edges; }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/mds/" + _name + ".mds";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("MDS save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("MDS save: cannot write " + path); return; }
            int m = (int)_edges.size() / 2;
            out << "NAME: " << _name << "\nTYPE: MDS\nVERTICES: " << _n << "\nEDGES: " << m << "\nEDGE_SECTION\n";
            for (int k = 0; k < m; ++k) out << (_edges[2 * k] + 1) << " " << (_edges[2 * k + 1] + 1) << "\n";  // 存 0 基 → 回写 1 基
        }

        Problem* getProblem()
        {
            if (_n == 0) return nullptr;
            // 度降序预排序(高度先) → 新序→原 的 _perm
            std::vector<int> deg(_n, 0);
            for (size_t k = 0; k < _edges.size(); k += 2) { deg[_edges[k]]++; deg[_edges[k + 1]]++; }
            _perm.resize(_n);
            for (int i = 0; i < _n; ++i) _perm[i] = i;
            std::stable_sort(_perm.begin(), _perm.end(), [&](int a, int b) { return deg[a] > deg[b]; });
            std::vector<int> inv(_n);
            for (int i = 0; i < _n; ++i) inv[_perm[i]] = i;
            std::vector<int> redges(_edges.size());
            for (size_t k = 0; k < _edges.size(); ++k) redges[k] = inv[_edges[k]];

            // 重标号 CSR(供覆盖 functor)
            std::vector<int> off(_n + 1, 0), rdeg(_n, 0);
            for (size_t k = 0; k < redges.size(); k += 2) { rdeg[redges[k]]++; rdeg[redges[k + 1]]++; }
            for (int i = 0; i < _n; ++i) off[i + 1] = off[i] + rdeg[i];
            std::vector<int> adj(off[_n], 0), cur(off.begin(), off.begin() + _n);
            for (size_t k = 0; k < redges.size(); k += 2)
            { int u = redges[k], w = redges[k + 1]; adj[cur[u]++] = w; adj[cur[w]++] = u; }

            Problem* back = new Problem(_name);
            back->addVariable("x", 0, 1, 1, _n);
            sizeFunc sf(_n);
            back->addObjective("size", 1, true, "x", &sf);          // 最小化 |S|
            double pen = (_penalty > 0) ? _penalty : 1e6;
            back->addConstrainGraphDominating("x", redges, pen, "size");
            coverFunc cf(std::move(off), std::move(adj));
            back->addInspirationFunc("x", "x", &cf);                // 读部分解
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "mds", "mds"));
            if (!f) { sys_logger.error("MDS instance not found: " + name); return; }
            int n = 0, m = -1; std::vector<int> e; std::string tok;
            while (f >> tok)
            {
                if (tok == "VERTICES:") f >> n;
                else if (tok == "EDGES:") f >> m;
                else if (tok == "EDGE_SECTION")          // 计数驱动:精确读 m 对,流保持 good
                {
                    if (m < 0) { sys_logger.error("MDS '" + name + "': EDGE_SECTION before EDGES count"); return; }
                    for (int i = 0; i < m; ++i)
                    {
                        int u, v;
                        if (!(f >> u >> v)) { sys_logger.error("MDS '" + name + "': EDGE_SECTION truncated, expected " + std::to_string(m) + " edges"); return; }
                        if (u < 1 || u > n || v < 1 || v > n) { sys_logger.error("MDS '" + name + "': edge endpoint out of [1," + std::to_string(n) + "]"); return; }
                        e.push_back(u - 1); e.push_back(v - 1);
                    }
                }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (n <= 0) { sys_logger.error("MDS '" + name + "': missing/invalid VERTICES"); return; }
            setGraph(n, e);
        }
    };
}
