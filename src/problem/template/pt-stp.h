//------------------------Description------------------------
// 斯坦纳树 (Steiner Tree Problem in Graphs) 问题模板。
//-------------------------Reference-------------------------
// 图上斯坦纳树;SteinLib / 第 11 届 DIMACS Challenge 实例(FrontierCO 的 STP 来源)。
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
    class PT_STP
    {
    public:
        enum class StpHeur { Spanning, SteinerPrune };   // 生成树(Kruskal)/ MST+剪非终端叶子(斯坦纳)
    private:
        std::string         _name = "stp";
        int                 _nv = 0;
        std::vector<int>    _etail, _ehead;        // 原边端点(0 基)
        std::vector<double> _ew;                   // 原边权
        std::vector<int>    _terminals;            // 终端(0 基)
        double              _penalty = 0;
        std::vector<int>    _eperm;                // 新边序 → 原边
        StpHeur             _heur = StpHeur::SteinerPrune;

        struct weightFunc : eccalcul_functor
        {
            std::vector<double> w;                 // 重标号边序的权
            explicit weightFunc(std::vector<double> w_) : w(std::move(w_)) {}
            double operator()(double** a) const { double s = 0; for (size_t i = 0; i < w.size(); ++i) s += w[i] * a[0][i]; return s; }
            eccalcul_functor* copy() { return new weightFunc(*this); }
        };
        // Kruskal 式：边 e 若连接两个不同分量则选。
        struct kruskalFunc : eccalcul_functor
        {
            int nv; std::vector<int> et, eh;       // 重标号边序的端点
            kruskalFunc(int n, std::vector<int> t, std::vector<int> h) : nv(n), et(std::move(t)), eh(std::move(h)) {}
            double operator()(double** in) const
            {
                int e = static_cast<int>(in[0][0] + 0.5);
                if (in[1][0] < 0.5) return 0.5;
                const double* x = in[2];                       // 部分解(边 <e 已定)
                std::vector<int> par(nv); for (int i = 0; i < nv; ++i) par[i] = i;
                auto find = [&](int a){ while (par[a] != a) { par[a] = par[par[a]]; a = par[a]; } return a; };
                for (int j = 0; j < e; ++j) if (x[j] >= 0.5) { int a = find(et[j]), b = find(eh[j]); if (a != b) par[a] = b; }
                return (find(et[e]) != find(eh[e])) ? 1.0 : 0.0;   // 连不同分量 → 选;否则跳(成环)
            }
            eccalcul_functor* copy() { return new kruskalFunc(*this); }
        };
        // 斯坦纳种子:复现预算的边集 S(MST+剪非终端叶子)。e∈S 选(=1),否则跳(=0)。
        struct steinerFunc : eccalcul_functor
        {
            std::vector<char> inS;             // 重标号边序:该边是否在斯坦纳树中
            explicit steinerFunc(std::vector<char> s) : inS(std::move(s)) {}
            double operator()(double** in) const
            {
                int e = static_cast<int>(in[0][0] + 0.5); double v = in[1][0];
                return inS[e] ? v : (1.0 - v);   // e∈S→偏好选(v=1);否则偏好跳(v=0);均非负
            }
            eccalcul_functor* copy() { return new steinerFunc(*this); }
        };

        // 预算斯坦纳种子:Kruskal MST → 反复剪掉"非终端叶子"边 → 连接终端的最小子树(MST 内)。
        std::vector<char> computeSteinerSeed() const
        {
            int m = static_cast<int>(_etail.size());
            std::vector<int> order(m); for (int e = 0; e < m; ++e) order[e] = e;
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return _ew[a] < _ew[b]; });
            std::vector<int> par(_nv); for (int i = 0; i < _nv; ++i) par[i] = i;
            auto find = [&](int a) { while (par[a] != a) { par[a] = par[par[a]]; a = par[a]; } return a; };
            std::vector<char> inMst(m, 0);
            for (int o : order) { int a = find(_etail[o]), b = find(_ehead[o]); if (a != b) { par[a] = b; inMst[o] = 1; } }
            std::vector<int> deg(_nv, 0); std::vector<std::vector<int>> adj(_nv);
            for (int e = 0; e < m; ++e) if (inMst[e]) { deg[_etail[e]]++; deg[_ehead[e]]++; adj[_etail[e]].push_back(e); adj[_ehead[e]].push_back(e); }
            std::vector<char> isTerm(_nv, 0); for (int t : _terminals) if (t >= 0 && t < _nv) isTerm[t] = 1;
            std::vector<char> removed(m, 0); std::vector<int> q;
            for (int v = 0; v < _nv; ++v) if (deg[v] == 1 && !isTerm[v]) q.push_back(v);
            for (size_t h = 0; h < q.size(); ++h)
            {
                int v = q[h]; if (deg[v] != 1 || isTerm[v]) continue;
                for (int e : adj[v]) if (inMst[e] && !removed[e])
                { removed[e] = 1; deg[v]--; int u = (_etail[e] == v) ? _ehead[e] : _etail[e]; if (--deg[u] == 1 && !isTerm[u]) q.push_back(u); break; }
            }
            std::vector<char> inS(m, 0);
            for (int e = 0; e < m; ++e) if (inMst[e] && !removed[e]) inS[e] = 1;
            return inS;
        }

    public:
        PT_STP() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return static_cast<int>(_etail.size()); }   // = |E|
        void setPenalty(double p) { _penalty = p; }
        void setHeuristic(StpHeur h) { _heur = h; }   // 默认 SteinerPrune(MST+剪枝)
        StpHeur getHeuristic() const { return _heur; }
        const std::vector<int>& getPermutation() const { return _eperm; }   // 新边序 → 原边

        // 内存注入：nv 顶点;edges 扁平 [u,v,...](0 基)+ 等长权 w;终端(0 基)。
        void setGraph(int nv, const std::vector<int>& edges, const std::vector<double>& w, const std::vector<int>& terminals)
        {
            _nv = nv; _ew = w; _terminals = terminals;
            int m = static_cast<int>(edges.size()) / 2;
            _etail.resize(m); _ehead.resize(m);
            for (int e = 0; e < m; ++e) { _etail[e] = edges[2 * e]; _ehead[e] = edges[2 * e + 1]; }
        }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/stp/" + _name + ".stp";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("STP save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("STP save: cannot write " + path); return; }
            int m = (int)_etail.size();
            out << "NAME: " << _name << "\nTYPE: STP\nNODES: " << _nv << "\nEDGES: " << m << "\nEDGE_SECTION\n";
            for (int e = 0; e < m; ++e) out << (_etail[e] + 1) << " " << (_ehead[e] + 1) << " " << _ew[e] << "\n";  // 0 基 → 1 基
            out << "TERMINALS: " << _terminals.size() << "\nTERMINAL_SECTION\n";
            for (int t : _terminals) out << (t + 1) << "\n";
        }

        Problem* getProblem()
        {
            int m = static_cast<int>(_etail.size());
            if (m == 0) return nullptr;
            // 按权升序预排序边 → 新边序 _eperm
            _eperm.resize(m); for (int e = 0; e < m; ++e) _eperm[e] = e;
            std::stable_sort(_eperm.begin(), _eperm.end(), [&](int a, int b) { return _ew[a] < _ew[b]; });

            std::vector<int> redges(2 * m), ret(m), reh(m); std::vector<double> rw(m);
            for (int e = 0; e < m; ++e)
            { int o = _eperm[e]; ret[e] = _etail[o]; reh[e] = _ehead[o]; rw[e] = _ew[o]; redges[2*e] = _etail[o]; redges[2*e+1] = _ehead[o]; }

            Problem* back = new Problem(_name);
            back->addVariable("y", 0, 1, 1, m);
            weightFunc wf(rw);
            back->addObjective("weight", 1, true, "y", &wf);        // 最小化总权
            double pen = (_penalty > 0) ? _penalty : 1e6;
            back->addConstrainGraphConnectivity("y", _nv, redges, _terminals, pen, "weight");
            if (_heur == StpHeur::SteinerPrune)
            {
                std::vector<char> inS = computeSteinerSeed();                  // 原边序
                std::vector<char> rinS(m); for (int e = 0; e < m; ++e) rinS[e] = inS[_eperm[e]];  // 重标号
                steinerFunc sf(std::move(rinS));
                back->addInspirationFunc("y", "", &sf);             // 复现斯坦纳种子
            }
            else
            {
                kruskalFunc kf(_nv, ret, reh);
                back->addInspirationFunc("y", "y", &kf);            // 生成树:读部分解
            }
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "stp", "stp"));
            if (!f) { sys_logger.error("STP instance not found: " + name); return; }
            int nv = 0, m = 0, nt = 0; bool ok = true; std::vector<int> e, term; std::vector<double> w; std::string tok;
            while (ok && f >> tok)
            {
                if      (tok == "NODES:")     f >> nv;
                else if (tok == "EDGES:")     f >> m;
                else if (tok == "TERMINALS:") f >> nt;
                else if (tok == "EDGE_SECTION")
                {
                    if (nv <= 0) { sys_logger.error("STP '" + name + "': EDGE_SECTION before NODES"); return; }
                    for (int i = 0; i < m && ok; ++i)
                    {
                        int u, v; double ww;
                        if (!(f >> u >> v >> ww)) { ok = false; break; }
                        if (u < 1 || u > nv || v < 1 || v > nv) { sys_logger.error("STP '" + name + "': edge endpoint out of [1," + std::to_string(nv) + "]"); return; }
                        e.push_back(u - 1); e.push_back(v - 1); w.push_back(ww);
                    }
                }
                else if (tok == "TERMINAL_SECTION")
                {
                    if (nv <= 0) { sys_logger.error("STP '" + name + "': TERMINAL_SECTION before NODES"); return; }
                    for (int i = 0; i < nt && ok; ++i)
                    {
                        int t; if (!(f >> t)) { ok = false; break; }
                        if (t < 1 || t > nv) { sys_logger.error("STP '" + name + "': terminal out of [1," + std::to_string(nv) + "]"); return; }
                        term.push_back(t - 1);
                    }
                }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("STP '" + name + "': section truncated"); return; }
            if (nv <= 0) { sys_logger.error("STP '" + name + "': missing/invalid NODES"); return; }
            setGraph(nv, e, w, term);
        }
    };
}
