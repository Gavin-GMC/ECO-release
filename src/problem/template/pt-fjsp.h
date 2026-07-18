//------------------------Description------------------------
// 柔性作业车间调度 (Flexible Job-Shop Scheduling Problem) 问题模板。
//-------------------------Reference-------------------------
// Brandimarte (1993) Routing and Scheduling in a Flexible Job Shop。
// FrontierCO 的 FJSP:Behnke & Geiger (2012) / Naderi & Roshanaei (2021) 实例。
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
    class PT_FJSP
    {
    public:
        enum class SeqMode { Const, Variable };   // 排序:常量(只优化路由) / 变量(联合优化)
        enum class SeqHeur { MWKR, Random };      // Variable 下排序启发式:MWKR / 随机
    private:
        static constexpr double INF = 1e18;
        std::string         _name = "fjsp";
        int                 _ops = 0, _M = 0;
        std::vector<double> _proc;            // [ops*M],禁用=INF
        std::vector<int>    _prev;            // [ops] 同作业前驱工序(全局号),无则 -1
        double              _penalty = 0;
        SeqMode             _seq = SeqMode::Const;
        SeqHeur             _seqHeur = SeqHeur::MWKR;

        // makespan:主动调度解码器。优先级来源 var? a[1] : cprio(常量)。a[0]=路由 x。
        //   反复在就绪工序(前驱已排/无前驱)中按 (优先级升序, 工序号升序) 选一个排上去。
        struct makespanFunc : eccalcul_functor
        {
            int ops, M; bool var;
            std::vector<double> proc, cprio; std::vector<int> prev;
            makespanFunc(int o, int m, bool v, std::vector<double> pr, std::vector<int> pv, std::vector<double> cp)
                : ops(o), M(m), var(v), proc(std::move(pr)), cprio(std::move(cp)), prev(std::move(pv)) {}
            double operator()(double** a) const
            {
                const double* x = a[0];
                std::vector<double> mach(M, 0), end(ops, 0);
                std::vector<char> done(ops, 0);
                double mk = 0;
                for (int sched = 0; sched < ops; ++sched)
                {
                    int best = -1; double bestp = 0;
                    for (int op = 0; op < ops; ++op)                            // 选就绪集中优先级最高者
                    {
                        if (done[op]) continue;
                        if (prev[op] >= 0 && !done[prev[op]]) continue;         // 前驱未排 → 未就绪
                        double pr = var ? a[1][op] : cprio[op];
                        if (best < 0 || pr < bestp) { bestp = pr; best = op; }  // 升序;平局保留更小 op(严格<)
                    }
                    int op = best, m = static_cast<int>(x[op] + 0.5);
                    double pt = proc[op * M + m];
                    if (pt >= INF * 0.5) pt = 0;                                // 禁用机器:置 0;违反惩罚由 eligible 约束提供
                    double ready = (prev[op] >= 0) ? end[prev[op]] : 0.0;
                    double st = std::max(ready, mach[m]);
                    double en = st + pt;
                    end[op] = en; mach[m] = en; done[op] = 1; if (en > mk) mk = en;
                }
                return mk;
            }
            eccalcul_functor* copy() { return new makespanFunc(*this); }
        };
        struct sptFunc : eccalcul_functor     // 偏好处理时间最短的机器(非负保序;禁用机器≈0 最低)
        {
            int M; std::vector<double> proc;
            sptFunc(int m, std::vector<double> p) : M(m), proc(std::move(p)) {}
            double operator()(double** in) const
            {
                int op = static_cast<int>(in[0][0] + 0.5), m = static_cast<int>(in[1][0] + 0.5);
                return 1.0 / (1.0 + proc[op * M + m]);   // proc 小→高;禁用(INF)→≈0
            }
            eccalcul_functor* copy() { return new sptFunc(*this); }
        };

        // 排序启发式 MWKR：按所属作业**剩余工作量**(用已定路由 x 算)降序 → 预排名 rank,
        //   偏好 s[op]=rank[op](rank 0=剩余最多=最先排)。读 in[2]=x;按构造 pass 缓存 rank。
        struct mwkrFunc : eccalcul_functor
        {
            int ops, M; std::vector<double> proc; std::vector<int> nextop;
            mutable std::vector<int> _rank; mutable int _lastDim = -1;
            mwkrFunc(int o, int m, std::vector<double> pr, const std::vector<int>& prev)
                : ops(o), M(m), proc(std::move(pr)), nextop(o, -1)
            { for (int op = 0; op < o; ++op) if (prev[op] >= 0) nextop[prev[op]] = op; }

            void recomputeRank(const double* x) const
            {
                std::vector<double> rem(ops, 0);
                for (int op = ops - 1; op >= 0; --op)                       // 逆序:后继先算
                {
                    int m = static_cast<int>(x[op] + 0.5);
                    double pt = proc[op * M + m]; if (pt >= INF * 0.5) pt = 0;
                    rem[op] = pt + (nextop[op] >= 0 ? rem[nextop[op]] : 0.0);
                }
                std::vector<int> idx(ops); for (int i = 0; i < ops; ++i) idx[i] = i;
                std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) { return rem[a] > rem[b]; });
                _rank.assign(ops, 0);
                for (int r = 0; r < ops; ++r) _rank[idx[r]] = r;            // rank 0 = 剩余最多
            }
            double operator()(double** in) const
            {
                int op = static_cast<int>(in[0][0] + 0.5);
                double v = in[1][0];
                if (op < _lastDim || _lastDim < 0) recomputeRank(in[2]);    // 新构造 pass → 重算 rank
                _lastDim = op;
                return 1.0 / (1.0 + std::fabs(v - _rank[op]));              // 偏好 v==rank[op],非负保序
            }
            eccalcul_functor* copy() { return new mwkrFunc(*this); }
        };

    public:
        PT_FJSP() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _ops; }
        void setPenalty(double p) { _penalty = p; }
        void setSeqMode(SeqMode m) { _seq = m; }     // 默认 Const(只优化路由)
        SeqMode getSeqMode() const { return _seq; }
        void setSeqHeur(SeqHeur h) { _seqHeur = h; } // Variable 下排序启发式(默认 MWKR)
        SeqHeur getSeqHeur() const { return _seqHeur; }

        // 内存注入：总工序数 ops、机器数 M、proc[ops*M](禁用=INF)、prev[ops]。
        void setData(int ops, int M, std::vector<double> proc, std::vector<int> prev)
        { _ops = ops; _M = M; _proc = std::move(proc); _prev = std::move(prev); }

        //   每作业一行 "nOps [nAlt (机器 时间)×nAlt]×nOps"(机器 1 基);禁用机器(proc==INF)不写入。overwrite=false 不覆盖。
        void save(bool overwrite = false)
        {
            std::string path = "_pdata/fjsp/" + _name + ".fjsp";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("FJSP save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("FJSP save: cannot write " + path); return; }
            std::vector<std::pair<int, int>> jobs;   // (首工序全局号, 工序数)
            for (int g = 0; g < _ops; ) { int start = g++, cnt = 1; while (g < _ops && _prev[g] != -1) { cnt++; g++; } jobs.push_back({ start, cnt }); }
            out << "NAME: " << _name << "\nTYPE: FJSP\nJOBS: " << jobs.size() << "\nMACHINES: " << _M << "\nJOB_SECTION\n";
            for (auto& jb : jobs)
            {
                out << jb.second;
                for (int o = 0; o < jb.second; ++o)
                {
                    int g = jb.first + o, nAlt = 0;
                    for (int m = 0; m < _M; ++m) if (_proc[(size_t)g * _M + m] < INF) nAlt++;
                    out << " " << nAlt;
                    for (int m = 0; m < _M; ++m) if (_proc[(size_t)g * _M + m] < INF) out << " " << (m + 1) << " " << _proc[(size_t)g * _M + m];
                }
                out << "\n";
            }
        }

        Problem* getProblem()
        {
            if (_ops == 0) return nullptr;
            Problem* back = new Problem(_name);
            back->addVariable("x", 0, _M - 1, 1, _ops);              // 路由(先注册→先决策)
            double pen = (_penalty > 0) ? _penalty : 1e6;
            if (_seq == SeqMode::Variable)
            {
                back->addVariable("s", 0, _ops - 1, 1, _ops);        // 排序优先级(后),允许并列,无约束
                makespanFunc mf(_ops, _M, true, _proc, _prev, {});
                back->addObjective("makespan", 1, true, "x,s", &mf); // 读 a[0]=x, a[1]=s
                if (_seqHeur == SeqHeur::MWKR)
                {
                    mwkrFunc sh(_ops, _M, _proc, _prev);
                    back->addInspirationFunc("s", "x", &sh);         // MWKR 预排名,读路由 x
                }
                // SeqHeur::Random:不注册 → s 默认 RandomInspiration
            }
            else
            {
                std::vector<double> cprio(_ops); for (int i = 0; i < _ops; ++i) cprio[i] = i;  // 全局工序序
                makespanFunc mf(_ops, _M, false, _proc, _prev, std::move(cprio));
                back->addObjective("makespan", 1, true, "x", &mf);
            }
            // 机器适用性(硬约束):每工序 x[op] 只能取其允许机器;违反由 eligible 的 violation 惩罚。
            std::vector<std::vector<double>> allowed(_ops);
            for (int op = 0; op < _ops; ++op)
                for (int m = 0; m < _M; ++m)
                    if (_proc[op * _M + m] < INF * 0.5) allowed[op].push_back(m);
            back->addConstrainEligible("x", allowed, pen, "makespan");
            sptFunc sf(_M, _proc);
            back->addInspirationFunc("x", "", &sf);                  // 路由启发式(排序启发式留待讨论)
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "fjsp", "fjsp"));
            if (!f) { sys_logger.error("FJSP instance not found: " + name); return; }
            int J = 0, M = 0; std::string tok; bool ok = true;
            std::vector<double> proc; std::vector<int> prev;
            int ops = 0;
            while (ok && f >> tok)
            {
                if (tok == "JOBS:") f >> J;
                else if (tok == "MACHINES:") f >> M;
                else if (tok == "JOB_SECTION")
                {
                    if (J <= 0 || M <= 0) { sys_logger.error("FJSP '" + name + "': JOB_SECTION before JOBS/MACHINES"); return; }
                    for (int j = 0; j < J && ok; ++j)
                    {
                        int nops; if (!(f >> nops)) { ok = false; break; }
                        for (int o = 0; o < nops && ok; ++o)
                        {
                            int gid = ops++;
                            proc.resize(static_cast<size_t>(ops) * M, INF);   // 逐工序追加一行(INF 填充)
                            prev.push_back(o == 0 ? -1 : gid - 1);
                            int nalt; if (!(f >> nalt)) { ok = false; break; }
                            for (int k = 0; k < nalt && ok; ++k)
                            {
                                int m; double t;
                                if (!(f >> m >> t)) { ok = false; break; }
                                if (m < 1 || m > M) { sys_logger.error("FJSP '" + name + "': machine out of [1," + std::to_string(M) + "]"); return; }
                                proc[static_cast<size_t>(gid) * M + (m - 1)] = t;
                            }
                        }
                    }
                }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            if (!ok) { sys_logger.error("FJSP '" + name + "': JOB_SECTION truncated"); return; }
            if (J <= 0 || M <= 0 || ops <= 0) { sys_logger.error("FJSP '" + name + "': missing JOBS/MACHINES/operations"); return; }
            setData(ops, M, proc, prev);
        }
    };
}
