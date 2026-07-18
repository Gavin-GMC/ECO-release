//------------------------Description------------------------
// 计算工作流调度 (Computational Workflow Scheduling) 问题模板。
//-------------------------Reference-------------------------
// 异构 DAG 调度 / HEFT(Topcuoglu et al. 2002, Heterogeneous Earliest-Finish-Time)。
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
#include <utility>
#include <algorithm>
#include <cmath>
#include "logger.hpp"

namespace ECFlow
{
    class PT_WFS
    {
    public:
        enum class SeqMode { Const, Variable };
        enum class SeqHeur { UpwardRank, Random };
        enum class MapHeur { Fastest, EFT };       // 映射启发式:最快机器 / HEFT 实时 EFT
        enum class ObjMode { MaxMakespan, AvgMakespan };   // 目标:各工作流完成时间的 最大 / 平均
    private:
        using Edge = std::pair<int, double>;   // (邻居任务, 数据量)
        std::string         _name = "wfs";
        int                 _n = 0, _M = 0;
        std::vector<double> _w;                // [n*M] 计算时间
        std::vector<int>    _eu, _ev;          // 边 u->v
        std::vector<double> _ed;               // 边数据量
        std::vector<double> _speed;            // [M*M] 机器对带宽(对角线占位)
        std::vector<double> _mem, _memCap, _storCap;  // 内存需求[n]/内存容量[M]/存储容量[M](空=不启用)
        std::vector<double> _deadline;          // 任务截止期[n](空=不启用)
        double              _penalty = 0;
        SeqMode             _seq = SeqMode::Const;
        SeqHeur             _seqHeur = SeqHeur::UpwardRank;
        MapHeur             _mapHeur = MapHeur::Fastest;
        ObjMode             _objMode = ObjMode::MaxMakespan;
        std::vector<int>    _perm;             // 内部重排:新序→原任务(拓扑序满足时为恒等)

        void buildAdj(std::vector<std::vector<Edge>>& pred, std::vector<std::vector<Edge>>& succ) const
        {
            pred.assign(_n, {}); succ.assign(_n, {});
            for (size_t e = 0; e < _eu.size(); ++e)
            { int u = _eu[e], v = _ev[e]; double d = _ed[e]; pred[v].push_back({u, d}); succ[u].push_back({v, d}); }
        }
        std::vector<int> topoOrder() const     // Kahn:前向拓扑序(源在前)
        {
            std::vector<int> indeg(_n, 0), order, q;
            std::vector<std::vector<int>> succ(_n);
            for (size_t e = 0; e < _eu.size(); ++e) { indeg[_ev[e]]++; succ[_eu[e]].push_back(_ev[e]); }
            for (int i = 0; i < _n; ++i) if (indeg[i] == 0) q.push_back(i);
            for (size_t h = 0; h < q.size(); ++h)
            { int u = q[h]; order.push_back(u); for (int v : succ[u]) if (--indeg[v] == 0) q.push_back(v); }
            return order;
        }

        // 目标:DAG 主动调度解码器。优先级 var? a[1] : cprio。a[0]=映射 x。
        struct makespanFunc : eccalcul_functor
        {
            int n, M; bool var, avgMode; int nWf; double pen;
            std::vector<double> w, cprio, speed, deadline; std::vector<int> comp;
            std::vector<std::vector<Edge>> pred;
            makespanFunc(int nn, int m, bool v, double p, std::vector<double> ww, std::vector<std::vector<Edge>> pr,
                         std::vector<double> sp, std::vector<double> cp, std::vector<double> dl,
                         bool avg, std::vector<int> cmp, int wf)
                : n(nn), M(m), var(v), avgMode(avg), nWf(wf), pen(p), w(std::move(ww)), cprio(std::move(cp)),
                  speed(std::move(sp)), deadline(std::move(dl)), comp(std::move(cmp)), pred(std::move(pr)) {}
            double operator()(double** a) const
            {
                const double* x = a[0];
                std::vector<double> mach(M, 0), fin(n, 0); std::vector<char> done(n, 0);
                double mk = 0;
                for (int s = 0; s < n; ++s)
                {
                    int best = -1; double bp = 0;
                    for (int i = 0; i < n; ++i)                                 // 就绪集中按优先级取
                    {
                        if (done[i]) continue;
                        bool ready = true; for (const Edge& e : pred[i]) if (!done[e.first]) { ready = false; break; }
                        if (!ready) continue;
                        double pr = var ? a[1][i] : cprio[i];
                        if (best < 0 || pr < bp) { bp = pr; best = i; }         // 升序,平局保留更小任务号
                    }
                    int i = best, m = static_cast<int>(x[i] + 0.5);
                    double st = mach[m];
                    for (const Edge& e : pred[i])                               // 前驱完工 + 通信送达
                    {
                        int k = e.first, mk2 = static_cast<int>(x[k] + 0.5);
                        double c = (mk2 == m) ? 0.0 : e.second / speed[mk2 * M + m];
                        double r = fin[k] + c; if (r > st) st = r;
                    }
                    double en = st + w[i * M + m];
                    fin[i] = en; mach[m] = en; done[i] = 1; if (en > mk) mk = en;
                }
                double obj = mk;                                        // MaxMakespan = 各工作流完成时间的最大 = 总 max
                if (avgMode && nWf > 0)                                 // AvgMakespan = 各工作流完成时间的平均
                {
                    std::vector<double> cmax(nWf, 0);
                    for (int i = 0; i < n; ++i) if (fin[i] > cmax[comp[i]]) cmax[comp[i]] = fin[i];
                    double s = 0; for (double c : cmax) s += c; obj = s / nWf;
                }
                mk = obj;
                if (!deadline.empty())                                  // 可选:截止期软惩罚
                {
                    double over = 0;
                    for (int i = 0; i < n; ++i) { double o = fin[i] - deadline[i]; if (o > 0) over += o; }
                    mk += pen * over;
                }
                return mk;
            }
            eccalcul_functor* copy() { return new makespanFunc(*this); }
        };

        // 映射启发式:最快机器(min w[i][m]),非负保序。
        struct mapFunc : eccalcul_functor
        {
            int M; std::vector<double> w;
            mapFunc(int m, std::vector<double> ww) : M(m), w(std::move(ww)) {}
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5), m = static_cast<int>(in[1][0] + 0.5);
                return 1.0 / (1.0 + w[i * M + m]);
            }
            eccalcul_functor* copy() { return new mapFunc(*this); }
        };

        // HEFT 实时 EFT 映射启发式:决定任务 i 时,把已映射任务 0..i-1 按构造序重放调度,
        //   候选机器 m 打分 1/(1+EFT),EFT=max(机器可用[m], max_前驱(finish+comm))+w[i][m]。
        //   读 in[2]=部分映射 x;按构造 pass 缓存调度。要求任务号拓扑序(前驱号 < 本任务号)。
        struct eftFunc : eccalcul_functor
        {
            int n, M; std::vector<double> w, speed; std::vector<std::vector<Edge>> pred;
            mutable std::vector<double> _fin; mutable std::vector<double> _avail; mutable int _lastDim = -1;
            eftFunc(int nn, int m, std::vector<double> ww, std::vector<double> sp, std::vector<std::vector<Edge>> pr)
                : n(nn), M(m), w(std::move(ww)), speed(std::move(sp)), pred(std::move(pr)), _fin(n, 0), _avail(M, 0) {}
            void reschedule(const double* x, int upto) const   // 重放 0..upto-1 的最早可行调度
            {
                std::fill(_avail.begin(), _avail.end(), 0.0);
                for (int k = 0; k < upto; ++k)
                {
                    int m = static_cast<int>(x[k] + 0.5);
                    double st = _avail[m];
                    for (const Edge& e : pred[k])
                    {
                        int p = e.first, mp = static_cast<int>(x[p] + 0.5);
                        double c = (mp == m) ? 0.0 : e.second / speed[mp * M + m];
                        double r = _fin[p] + c; if (r > st) st = r;
                    }
                    _fin[k] = st + w[k * M + m]; _avail[m] = _fin[k];
                }
            }
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5), m = static_cast<int>(in[1][0] + 0.5);
                const double* x = in[2];
                if (i != _lastDim) { reschedule(x, i); _lastDim = i; }
                double st = _avail[m];
                for (const Edge& e : pred[i])
                {
                    int p = e.first, mp = static_cast<int>(x[p] + 0.5);
                    double c = (mp == m) ? 0.0 : e.second / speed[mp * M + m];
                    double r = _fin[p] + c; if (r > st) st = r;
                }
                double eft = st + w[i * M + m];
                return 1.0 / (1.0 + eft);                       // 低 EFT → 高分
            }
            eccalcul_functor* copy() { return new eftFunc(*this); }
        };

        // 排序启发式:向上秩(HEFT)。ur[i]=w[i][x[i]]+max_{后继 j}(comm(i,j)+ur[j]) → 降序预排名。读 in[2]=x。
        struct urankFunc : eccalcul_functor
        {
            int n, M; std::vector<double> w, speed; std::vector<std::vector<Edge>> succ; std::vector<int> topo;
            mutable std::vector<int> _rank; mutable int _lastDim = -1;
            urankFunc(int nn, int m, std::vector<double> ww, std::vector<double> sp,
                      std::vector<std::vector<Edge>> sc, std::vector<int> tp)
                : n(nn), M(m), w(std::move(ww)), speed(std::move(sp)), succ(std::move(sc)), topo(std::move(tp)) {}
            void recomputeRank(const double* x) const
            {
                std::vector<double> ur(n, 0);
                for (int t = n - 1; t >= 0; --t)                               // 逆拓扑:后继先算
                {
                    int i = topo[t], mi = static_cast<int>(x[i] + 0.5);
                    double best = 0;
                    for (const Edge& e : succ[i])
                    {
                        int j = e.first, mj = static_cast<int>(x[j] + 0.5);
                        double c = (mi == mj) ? 0.0 : e.second / speed[mi * M + mj];
                        double val = c + ur[j]; if (val > best) best = val;
                    }
                    ur[i] = w[i * M + mi] + best;
                }
                std::vector<int> idx(n); for (int i = 0; i < n; ++i) idx[i] = i;
                std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) { return ur[a] > ur[b]; });
                _rank.assign(n, 0); for (int r = 0; r < n; ++r) _rank[idx[r]] = r;
            }
            double operator()(double** in) const
            {
                int i = static_cast<int>(in[0][0] + 0.5); double v = in[1][0];
                if (i < _lastDim || _lastDim < 0) recomputeRank(in[2]);
                _lastDim = i;
                return 1.0 / (1.0 + std::fabs(v - _rank[i]));
            }
            eccalcul_functor* copy() { return new urankFunc(*this); }
        };

    public:
        PT_WFS() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _n; }
        void setPenalty(double p) { _penalty = p; }
        void setSeqMode(SeqMode m) { _seq = m; }
        SeqMode getSeqMode() const { return _seq; }
        void setSeqHeur(SeqHeur h) { _seqHeur = h; }
        SeqHeur getSeqHeur() const { return _seqHeur; }
        void setMapHeur(MapHeur h) { _mapHeur = h; }   // 映射启发式(默认 Fastest;EFT=HEFT 实时)
        MapHeur getMapHeur() const { return _mapHeur; }
        void setObjMode(ObjMode m) { _objMode = m; }   // 目标:Max(默认)/ Avg 各工作流完成时间
        ObjMode getObjMode() const { return _objMode; }
        const std::vector<int>& getPermutation() const { return _perm; }   // 新序→原任务(getProblem 后有效)

        // n 任务、M 机器;w[n*M];边 (eu,ev,ed);speed[M*M]。
        void setData(int n, int M, std::vector<double> w, std::vector<int> eu, std::vector<int> ev,
                     std::vector<double> ed, std::vector<double> speed)
        { _n = n; _M = M; _w = std::move(w); _eu = std::move(eu); _ev = std::move(ev); _ed = std::move(ed); _speed = std::move(speed); }

        // 资源(可选):内存需求 mem[n]、内存容量 memCap[M]、存储容量 storCap[M]。
        void setResources(std::vector<double> mem, std::vector<double> memCap, std::vector<double> storCap)
        { _mem = std::move(mem); _memCap = std::move(memCap); _storCap = std::move(storCap); }

        // 截止期(可选):deadline[n]。空 = 不启用 deadline 惩罚。
        void setDeadline(std::vector<double> deadline) { _deadline = std::move(deadline); }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/wfs/" + _name + ".wfs";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("WFS save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("WFS save: cannot write " + path); return; }
            int E = (int)_eu.size();
            out << "NAME: " << _name << "\nTYPE: WFS\nNODES: " << _n << "\nMACHINES: " << _M << "\nEDGES: " << E << "\n";
            out << "COMP_SECTION\n";  for (int i = 0; i < _n; ++i) { out << (i + 1); for (int m = 0; m < _M; ++m) out << " " << _w[(size_t)i * _M + m]; out << "\n"; }
            out << "EDGE_SECTION\n";  for (int e = 0; e < E; ++e) out << (_eu[e] + 1) << " " << (_ev[e] + 1) << " " << _ed[e] << "\n";  // 0 基 → 1 基
            out << "SPEED_SECTION\n"; for (int a = 0; a < _M; ++a) { out << (a + 1); for (int b = 0; b < _M; ++b) out << " " << _speed[(size_t)a * _M + b]; out << "\n"; }
            if ((int)_mem.size()      == _n) { out << "MEM_SECTION\n";      for (int i = 0; i < _n; ++i) out << (i + 1) << " " << _mem[i]      << "\n"; }
            if ((int)_memCap.size()   == _M) { out << "MEMCAP_SECTION\n";   for (int m = 0; m < _M; ++m) out << (m + 1) << " " << _memCap[m]   << "\n"; }
            if ((int)_storCap.size()  == _M) { out << "STORCAP_SECTION\n";  for (int m = 0; m < _M; ++m) out << (m + 1) << " " << _storCap[m]  << "\n"; }
            if ((int)_deadline.size() == _n) { out << "DEADLINE_SECTION\n"; for (int i = 0; i < _n; ++i) out << (i + 1) << " " << _deadline[i] << "\n"; }
        }

        Problem* getProblem()
        {
            if (_n == 0) return nullptr;

            // 1) 拓扑序断言:每条边 u<v。满足→恒等;不满足→按拓扑序重排 + 日志警告。
            _perm.resize(_n);
            bool topoOk = true;
            for (size_t e = 0; e < _eu.size(); ++e) if (_eu[e] >= _ev[e]) { topoOk = false; break; }
            if (topoOk) { for (int i = 0; i < _n; ++i) _perm[i] = i; }
            else
            {
                _perm = topoOrder();   // 新序→原任务
                if ((int)_perm.size() != _n) { sys_logger.error("WFS '" + _name + "': graph not a DAG (cycle)"); return nullptr; }
                sys_logger.warning("WFS '" + _name + "': task ids not topological -> internally relabeled to topological order (getPermutation maps new->old).");
            }
            std::vector<int> inv(_n); for (int k = 0; k < _n; ++k) inv[_perm[k]] = k;

            // 2) 按 _perm 重排数据(机器维数据 speed/memCap/storCap 不变;边数据 ed 不变,端点重映射)。
            std::vector<double> w(static_cast<size_t>(_n) * _M);
            for (int k = 0; k < _n; ++k) for (int m = 0; m < _M; ++m) w[k * _M + m] = _w[_perm[k] * _M + m];
            std::vector<double> deadline;   // 可选,按 _perm 重排
            if (!_deadline.empty()) { deadline.resize(_n); for (int k = 0; k < _n; ++k) deadline[k] = _deadline[_perm[k]]; }
            // 工作流 = 弱连通分量(并查集,重排空间)
            std::vector<int> uf(_n); for (int i = 0; i < _n; ++i) uf[i] = i;
            auto ufind = [&uf](int a) { while (uf[a] != a) { uf[a] = uf[uf[a]]; a = uf[a]; } return a; };
            for (size_t e = 0; e < _eu.size(); ++e) { int a = ufind(inv[_eu[e]]), b = ufind(inv[_ev[e]]); if (a != b) uf[a] = b; }
            std::vector<int> comp(_n, -1); int nWf = 0;
            for (int i = 0; i < _n; ++i) { int r = ufind(i); if (comp[r] < 0) comp[r] = nWf++; }
            for (int i = 0; i < _n; ++i) comp[i] = comp[ufind(i)];
            bool avg = (_objMode == ObjMode::AvgMakespan);
            std::vector<std::vector<Edge>> pred(_n), succ(_n);
            for (size_t e = 0; e < _eu.size(); ++e)
            { int u = inv[_eu[e]], v = inv[_ev[e]]; double d = _ed[e]; pred[v].push_back({u, d}); succ[u].push_back({v, d}); }
            std::vector<int> topo(_n); for (int i = 0; i < _n; ++i) topo[i] = i;   // 重排后 id 序即拓扑序

            // 3) 构建问题(全用重排后的 w/pred/succ)
            Problem* back = new Problem(_name);
            back->addVariable("x", 0, _M - 1, 1, _n);                // 映射(先注册→先决策)
            double pen = (_penalty > 0) ? _penalty : 1e6;
            if (_seq == SeqMode::Variable)
            {
                back->addVariable("s", 0, _n - 1, 1, _n);            // 排序优先级(后),允许并列
                makespanFunc mf(_n, _M, true, pen, w, pred, _speed, {}, deadline, avg, comp, nWf);
                back->addObjective("makespan", 1, true, "x,s", &mf);
                if (_seqHeur == SeqHeur::UpwardRank)
                {
                    urankFunc sh(_n, _M, w, _speed, succ, topo);
                    back->addInspirationFunc("s", "x", &sh);         // 向上秩,读映射 x
                }
                // SeqHeur::Random:不注册 → s 默认 RandomInspiration
            }
            else
            {
                std::vector<double> cprio(_n); for (int i = 0; i < _n; ++i) cprio[i] = i;   // 任务号常量
                makespanFunc mf(_n, _M, false, pen, w, pred, _speed, std::move(cprio), deadline, avg, comp, nWf);
                back->addObjective("makespan", 1, true, "x", &mf);
            }
            // 内存:适用性(mem 按 _perm 重排)
            if (!_mem.empty() && !_memCap.empty())
            {
                std::vector<std::vector<double>> allowed(_n);
                for (int i = 0; i < _n; ++i)
                { double mi = _mem[_perm[i]]; for (int m = 0; m < _M; ++m) if (_memCap[m] >= mi) allowed[i].push_back(m); }
                back->addConstrainEligible("x", allowed, pen, "makespan");
            }
            // 存储:容量(outdata 从重排后边算)
            if (!_storCap.empty())
            {
                std::vector<double> outdata(_n, 0);
                for (int v = 0; v < _n; ++v) for (const Edge& e : succ[v]) outdata[v] += e.second;
                back->addConstrainCapacity("x", _storCap.data(), _M, outdata.data(), _n, pen, "makespan");
            }
            if (_mapHeur == MapHeur::EFT)
            {
                eftFunc mp(_n, _M, w, _speed, pred);
                back->addInspirationFunc("x", "x", &mp);             // HEFT 实时 EFT(读部分映射 x)
            }
            else
            {
                mapFunc mp(_M, w);
                back->addInspirationFunc("x", "", &mp);              // 最快机器(min w)
            }
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "wfs", "wfs"));
            if (!f) { sys_logger.error("WFS instance not found: " + name); return; }
            int n = 0, M = 0, E = -1; std::string tok; bool ok = true;
            std::vector<double> w, ed, speed, mem, memCap, storCap, deadline; std::vector<int> eu, ev;
            // 统一行号规范:向量/矩阵段每逻辑行以 1-based 索引开头(读入即忽略);EDGE_SECTION 是边表、不加索引。
            std::string idx;
            auto rdv = [&](std::vector<double>& v, int cnt) { v.assign(cnt, 0); for (int i = 0; i < cnt && ok; ++i) if (!(f >> idx >> v[i])) ok = false; };
            auto rdm = [&](std::vector<double>& v, int rows, int cols) { v.assign((size_t)rows * cols, 0); for (int r = 0; r < rows && ok; ++r) { if (!(f >> idx)) { ok = false; break; } for (int cc = 0; cc < cols && ok; ++cc) if (!(f >> v[(size_t)r * cols + cc])) ok = false; } };
            while (ok && f >> tok)
            {
                if (tok == "NODES:") f >> n;
                else if (tok == "MACHINES:") f >> M;
                else if (tok == "EDGES:") f >> E;
                else if (tok == "COMP_SECTION")  { if (n <= 0 || M <= 0) { sys_logger.error("WFS '" + name + "': COMP_SECTION before NODES/MACHINES"); return; } rdm(w, n, M); }
                else if (tok == "SPEED_SECTION") { if (M <= 0)          { sys_logger.error("WFS '" + name + "': SPEED_SECTION before MACHINES"); return; } rdm(speed, M, M); }
                else if (tok == "MEM_SECTION")     { if (n <= 0) { sys_logger.error("WFS '" + name + "': MEM_SECTION before NODES"); return; }     rdv(mem, n); }
                else if (tok == "MEMCAP_SECTION")  { if (M <= 0) { sys_logger.error("WFS '" + name + "': MEMCAP_SECTION before MACHINES"); return; } rdv(memCap, M); }
                else if (tok == "STORCAP_SECTION") { if (M <= 0) { sys_logger.error("WFS '" + name + "': STORCAP_SECTION before MACHINES"); return; } rdv(storCap, M); }
                else if (tok == "DEADLINE_SECTION") { if (n <= 0) { sys_logger.error("WFS '" + name + "': DEADLINE_SECTION before NODES"); return; } rdv(deadline, n); }
                else if (tok == "EDGE_SECTION")
                {
                    if (E < 0 || n <= 0) { sys_logger.error("WFS '" + name + "': EDGE_SECTION before EDGES/NODES"); return; }
                    for (int e = 0; e < E && ok; ++e)
                    {
                        int u, v; double d;
                        if (!(f >> u >> v >> d)) { ok = false; break; }
                        if (u < 1 || u > n || v < 1 || v > n) { sys_logger.error("WFS '" + name + "': edge endpoint out of [1," + std::to_string(n) + "]"); return; }
                        eu.push_back(u - 1); ev.push_back(v - 1); ed.push_back(d);
                    }
                }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("WFS '" + name + "': section truncated"); return; }
            if (n <= 0 || M <= 0) { sys_logger.error("WFS '" + name + "': missing NODES/MACHINES"); return; }
            setData(n, M, w, eu, ev, ed, speed);
            if (!mem.empty() || !storCap.empty()) setResources(mem, memCap, storCap);   // 资源段可选
            if (!deadline.empty()) setDeadline(deadline);                                // 截止期段可选
        }
    };
}
