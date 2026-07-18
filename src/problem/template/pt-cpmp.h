//------------------------Description------------------------
// 带容量 p-中位 (Capacitated p-Median Problem) 问题模板。
//-------------------------Reference-------------------------
// Lorena & Senne、Stefanello et al.、Gnägi & Baumann 的 CPMP test bed(FrontierCO 的 CPMP 来源)。
// 启发式参考:Mulvey & Beck (1984) regret 构造;Ahmadi & Osman (2004) 密度型 desirability。
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
#include <cmath>
#include "logger.hpp"

namespace ECFlow
{
    class PT_CPMP
    {
    public:
        enum class Heuristic { GreedySeed, Composite };
    private:
        static constexpr int KNN_LIMIT = 5000;   // N 超此值则 Composite 关闭 k-近邻(密度/覆盖项)
        std::string         _name = "cpmp";
        int                 _N = 0, _p = 0;
        std::vector<double> _cap, _dem, _d;       // cap[N], dem[N], d[N*N]
        double              _penalty = 0;
        Heuristic           _heur = Heuristic::GreedySeed;

        // 目标:纯距离 Σ d[i][x[i]](容量/p 由约束惩罚承担)。
        struct distFunc : eccalcul_functor
        {
            int N; std::vector<double> d;
            distFunc(int n, std::vector<double> dd) : N(n), d(std::move(dd)) {}
            double operator()(double** a) const
            {
                double s = 0;
                for (int i = 0; i < N; ++i) { int j = static_cast<int>(a[0][i] + 0.5); s += d[i * N + j]; }
                return s;
            }
            eccalcul_functor* copy() { return new distFunc(*this); }
        };

        // GreedySeed:偏好参考解中位集 M_g(在其中加常数 bonus),距离破平。
        struct seedFunc : eccalcul_functor
        {
            int N; std::vector<double> d; std::vector<char> Mg;
            seedFunc(int n, std::vector<double> dd, std::vector<char> m) : N(n), d(std::move(dd)), Mg(std::move(m)) {}
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5), j = static_cast<int>(in[1][0] + 0.5);
                double base = 1.0 / (1.0 + d[i * N + j]);
                return Mg[j] ? (1.0 + base) : base;
            }
            eccalcul_functor* copy() { return new seedFunc(*this); }
        };

        // Composite:密度型 desirability。归一化 + 相位调度 + 可选覆盖项(k-近邻)。
        struct compositeFunc : eccalcul_functor
        {
            int N, p; bool cov;
            std::vector<double> d, cap, dem, neighDem;   // neighDem[N](仅 cov 时)
            std::vector<int>    nbrOff, nbr;             // k-近邻 CSR(仅 cov 时)
            double capMax = 1, ndMax = 1;
            // 默认权重:选址相位 (距离/容量/密度/覆盖) + 分配相位 (距离/容量)
            double wd_s = 1.0, wc_s = 0.5, wn_s = 1.0, wm_s = 0.5;
            double wd_a = 1.0, wc_a = 0.5;
            // 按维度缓存的运行态(同一维的所有候选共用,避免每候选重扫 in[2])
            mutable int                _cachedDim = -1;
            mutable std::vector<double> _resid;
            mutable std::vector<char>   _open;
            mutable int                _nOpen = 0;

            compositeFunc(int n, int pp, bool c, std::vector<double> dd, std::vector<double> ca,
                          std::vector<double> de, std::vector<double> nd,
                          std::vector<int> off, std::vector<int> nb, double cm, double ndm)
                : N(n), p(pp), cov(c), d(std::move(dd)), cap(std::move(ca)), dem(std::move(de)),
                  neighDem(std::move(nd)), nbrOff(std::move(off)), nbr(std::move(nb)), capMax(cm), ndMax(ndm),
                  _resid(N, 0), _open(N, 0) {}

            void refresh(int dim, const double* x) const
            {
                for (int j = 0; j < N; ++j) { _resid[j] = cap[j]; _open[j] = 0; }
                _nOpen = 0;
                for (int k = 0; k < dim; ++k)
                { int v = static_cast<int>(x[k] + 0.5); _resid[v] -= dem[k]; if (!_open[v]) { _open[v] = 1; ++_nOpen; } }
                _cachedDim = dim;
            }
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5), j = static_cast<int>(in[1][0] + 0.5);
                const double* x = in[2];
                if (_cachedDim != i) refresh(i, x);
                double nd = 1.0 / (1.0 + d[i * N + j]);
                if (_nOpen < p)            // 选址相位:已开中位按距离+余量复用;开新中位另加密度−覆盖+名额奖励
                {
                    double s = wd_s * nd + wc_s * (_resid[j] / capMax);
                    if (!_open[j] && cov)  // 开新中位:加密度 − 覆盖冗余
                    {
                        s += wn_s * (neighDem[j] / ndMax);
                        double c = 0;
                        for (int t = nbrOff[j]; t < nbrOff[j + 1]; ++t) { int m = nbr[t]; if (_open[m]) c += cap[m]; }
                        s -= wm_s * (c / ndMax);
                    }
                    return s;
                }
                return wd_a * nd + wc_a * (_resid[j] / capMax);   // 分配相位:距离 + 容量余量
            }
            eccalcul_functor* copy() { return new compositeFunc(*this); }
        };

        // Mulvey-Beck-lite:farthest-first 选址 + regret 分配 + 1 次重心化 → 中位标记 Mg[N]。
        std::vector<char> computeGreedySeed() const
        {
            int N = _N, p = _p;
            std::vector<int> med;
            int first = 0; for (int j = 1; j < N; ++j) if (_cap[j] > _cap[first]) first = j;
            med.push_back(first);
            std::vector<double> mind(N); for (int j = 0; j < N; ++j) mind[j] = _d[j * N + first];
            std::vector<char> chosen(N, 0); chosen[first] = 1;
            while ((int)med.size() < p)
            {
                int far = -1; double best = -1;
                for (int j = 0; j < N; ++j) if (!chosen[j] && mind[j] > best) { best = mind[j]; far = j; }
                if (far < 0) break;
                chosen[far] = 1; med.push_back(far);
                for (int j = 0; j < N; ++j) mind[j] = std::min(mind[j], _d[j * N + far]);
            }
            // regret 分配
            std::vector<int> order(N); for (int i = 0; i < N; ++i) order[i] = i;
            std::vector<double> regret(N);
            for (int i = 0; i < N; ++i)
            {
                double d1 = 1e18, d2 = 1e18;
                for (int m : med) { double dd = _d[i * N + m]; if (dd < d1) { d2 = d1; d1 = dd; } else if (dd < d2) d2 = dd; }
                regret[i] = (d2 >= 1e17) ? 0.0 : d2 - d1;
            }
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return regret[a] > regret[b]; });
            std::vector<double> resid(N, 0); for (int m : med) resid[m] = _cap[m];
            std::vector<int> assign(N, -1);
            for (int i : order)
            {
                int bm = -1; double bd = 1e18;
                for (int m : med) if (resid[m] >= _dem[i] && _d[i * N + m] < bd) { bd = _d[i * N + m]; bm = m; }
                if (bm < 0) for (int m : med) if (_d[i * N + m] < bd) { bd = _d[i * N + m]; bm = m; }
                assign[i] = bm; resid[bm] -= _dem[i];
            }
            // 1 次重心化:每簇取簇内距离和最小者为新中位
            std::vector<std::vector<int>> cl(N);
            for (int i = 0; i < N; ++i) if (assign[i] >= 0) cl[assign[i]].push_back(i);
            std::vector<char> Mg(N, 0);
            for (int m : med)
            {
                if (cl[m].empty()) { Mg[m] = 1; continue; }
                int bc = cl[m][0]; double bs = 1e18;
                for (int c : cl[m]) { double s = 0; for (int o : cl[m]) s += _d[c * N + o]; if (s < bs) { bs = s; bc = c; } }
                Mg[bc] = 1;
            }
            return Mg;
        }

    public:
        PT_CPMP() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _N; }
        void setPenalty(double p) { _penalty = p; }
        void setHeuristic(Heuristic h) { _heur = h; }     // 默认 GreedySeed
        Heuristic getHeuristic() const { return _heur; }

        void setData(int N, int p, std::vector<double> cap, std::vector<double> dem, std::vector<double> d)
        { _N = N; _p = p; _cap = std::move(cap); _dem = std::move(dem); _d = std::move(d); }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/cpmp/" + _name + ".cpmp";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("CPMP save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("CPMP save: cannot write " + path); return; }
            out << "NAME: " << _name << "\nTYPE: CPMP\nNODES: " << _N << "\nMEDIANS: " << _p << "\n";
            out << "CAPACITY_SECTION\n"; for (int j = 0; j < _N; ++j) out << (j + 1) << " " << _cap[j] << "\n";
            out << "DEMAND_SECTION\n";   for (int i = 0; i < _N; ++i) out << (i + 1) << " " << _dem[i] << "\n";
            out << "DISTANCE_SECTION\n";
            for (int i = 0; i < _N; ++i) { out << (i + 1); for (int j = 0; j < _N; ++j) out << " " << _d[(size_t)i * _N + j]; out << "\n"; }
        }

        Problem* getProblem()
        {
            if (_N == 0) return nullptr;
            Problem* back = new Problem(_name);
            back->addVariable("x", 0, _N - 1, 1, _N, 1, VariableType::allocation);   // 每节点→中位(单源)
            double pen = (_penalty > 0) ? _penalty : 1e6;
            distFunc df(_N, _d);
            back->addObjective("cost", 1, true, "x", &df);                            // 最小化总分配距离
            back->addConstrainCapacity("x", _cap.data(), _N, _dem.data(), _N, pen, "cost");  // 容量(硬)
            back->addConstrainDistinctCap("x", _p, pen, "cost");                      // 至多 p 个中位 + |·−p| 惩罚

            // 懒加载:只构造被选中的启发式 functor 及其预计算。
            if (_heur == Heuristic::GreedySeed)
            {
                seedFunc sf(_N, _d, computeGreedySeed());
                back->addInspirationFunc("x", "x", &sf);
            }
            else
            {
                bool cov = (_N <= KNN_LIMIT);
                double capMax = 1; for (int j = 0; j < _N; ++j) capMax = std::max(capMax, _cap[j]);
                std::vector<int> nbrOff, nbr; std::vector<double> neighDem; double ndMax = 1;
                if (cov)
                {
                    int k = std::min(_N - 1, 20);
                    nbrOff.assign(_N + 1, 0); neighDem.assign(_N, 0);
                    std::vector<int> idx(_N);
                    for (int j = 0; j < _N; ++j)
                    {
                        for (int t = 0; t < _N; ++t) idx[t] = t;
                        std::partial_sort(idx.begin(), idx.begin() + std::min(_N, k + 1), idx.end(),
                                          [&](int a, int b) { return _d[j * _N + a] < _d[j * _N + b]; });
                        double s = 0; int cnt = 0;
                        for (int t = 0; t < _N && cnt < k; ++t) { int m = idx[t]; if (m == j) continue; s += _dem[m]; nbr.push_back(m); ++cnt; }
                        neighDem[j] = s; nbrOff[j + 1] = (int)nbr.size();
                        ndMax = std::max(ndMax, s);
                    }
                }
                compositeFunc cf(_N, _p, cov, _d, _cap, _dem, std::move(neighDem),
                                 std::move(nbrOff), std::move(nbr), capMax, ndMax);
                back->addInspirationFunc("x", "x", &cf);
            }
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "cpmp", "cpmp"));
            if (!f) { sys_logger.error("CPMP instance not found: " + name); return; }
            int N = 0, p = 0; std::string tok; bool ok = true;
            std::vector<double> cap, dem, d;
            auto need = [&]() -> bool   // 段必须在 NODES 计数之后
            { if (N <= 0) { sys_logger.error("CPMP '" + name + "': section before NODES"); return false; } return true; };
            // 统一行号规范:每逻辑行以 1-based 索引开头(读入即忽略)。向量=每元素一行 `idx val`;矩阵=每行 `idx v0..v_{N-1}`。
            std::string idx;
            auto rdv = [&](std::vector<double>& v, int n) { v.assign(n, 0); for (int i = 0; i < n && ok; ++i) if (!(f >> idx >> v[i])) ok = false; };
            auto rdm = [&](std::vector<double>& v, int rows, int cols) { v.assign((size_t)rows * cols, 0); for (int r = 0; r < rows && ok; ++r) { if (!(f >> idx)) { ok = false; break; } for (int cc = 0; cc < cols && ok; ++cc) if (!(f >> v[(size_t)r * cols + cc])) ok = false; } };
            while (ok && f >> tok)
            {
                if (tok == "NODES:") f >> N;
                else if (tok == "MEDIANS:") f >> p;
                else if (tok == "CAPACITY_SECTION") { if (!(ok = need())) break; rdv(cap, N); }
                else if (tok == "DEMAND_SECTION")   { if (!(ok = need())) break; rdv(dem, N); }
                else if (tok == "DISTANCE_SECTION") { if (!(ok = need())) break; rdm(d, N, N); }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("CPMP '" + name + "': section truncated"); return; }
            if (N <= 0 || p <= 0 || p > N) { sys_logger.error("CPMP '" + name + "': invalid NODES/MEDIANS"); return; }
            setData(N, p, cap, dem, d);
        }
    };
}
