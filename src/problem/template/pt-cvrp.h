//------------------------Description------------------------
// 带容量车辆路径 (Capacitated Vehicle Routing Problem) 问题模板。
//-------------------------Reference-------------------------
// Prins C. (2004) A simple and effective evolutionary algorithm for the VRP.
// Vidal T. (2016) Technical note: Split algorithm in O(n) for the CVRP.
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
#include <deque>
#include <algorithm>
#include <cmath>
#include "logger.hpp"

namespace ECFlow
{
    class PT_CVRP
    {
    public:
        // 切段(拆分)策略：Greedy=贪心顺序切(O(N),给定顺序次优);
        //   Optimal=Prins 最优拆分 DP(O(N·B),给定顺序全局最优);
        //   Linear =Vidal 2016 单调队列(O(N),与 Optimal 同解)。
        enum class SplitMode { Greedy, Optimal, Linear };
    private:
        std::string         _name = "cvrp";
        int                 _N = 0;            // 客户数(不含车场)
        std::vector<double> _x, _y;            // 坐标 [N+1](下标 0=车场)
        std::vector<double> _dem;              // 需求 [N+1](_dem[0]=0)
        double              _Q = 0;            // 容量
        double              _penalty = 0;
        SplitMode           _split = SplitMode::Optimal;   // 默认最优拆分
        int                 _K = 0;                        // 车辆数上限(0=不限)

        static double dist(double ax, double ay, double bx, double by)
        { double dx = ax - bx, dy = ay - by; return std::sqrt(dx * dx + dy * dy); }

        // 目标:按排列分路求总距离(节点 = 客户号+1;0=车场)。切段策略可切换。
        //   车辆数 K:K=0 不限;K>0 时在「至多 K 条路」约束下划分——
        //     Greedy 取容量最少路数,超 K 即不可行(INF);
        //     Optimal/Linear 做 K 受限最优拆分(加路数维),取 k≤K 最小成本,无可行解返回 INF。
        //   单客户需求 > Q 时该客户无法成路:Optimal/Linear 返回 INF;Greedy 仍"超载单独成路"。
        struct routeFunc : eccalcul_functor
        {
            static constexpr double INF = 1e18;
            int N; double Q; int K; std::vector<double> x, y, dem; SplitMode mode;
            routeFunc(int n, double q, int k, std::vector<double> xs, std::vector<double> ys, std::vector<double> d, SplitMode m)
                : N(n), Q(q), K(k), x(std::move(xs)), y(std::move(ys)), dem(std::move(d)), mode(m) {}

            double C(int u, int v) const { return dist(x[u], y[u], x[v], y[v]); }   // 节点间距

            double operator()(double** a) const
            {
                std::vector<int> nd(N);
                for (int i = 0; i < N; ++i) nd[i] = static_cast<int>(a[0][i] + 0.5) + 1;   // 位置 i 的节点
                switch (mode)
                {
                    case SplitMode::Greedy:  return greedy(nd);
                    case SplitMode::Linear:  return linear(nd);
                    case SplitMode::Optimal: default: return optimal(nd);
                }
            }

            // ① 贪心顺序切：累计超容量即回车场另开一路。贪心路数 = 该顺序的**最少**路数,
            //    故 K>0 且路数 > K ⇒ 该顺序无 ≤K 划分 ⇒ 不可行(INF)。
            double greedy(const std::vector<int>& nd) const
            {
                if (N == 0) return 0.0;
                double total = 0, load = 0; int prev = 0, routes = 1;
                for (int i = 0; i < N; ++i)
                {
                    int node = nd[i];
                    if (load + dem[node] > Q + 1e-9) { total += C(prev, 0); prev = 0; load = 0; ++routes; }
                    total += C(prev, node); prev = node; load += dem[node];
                }
                total += C(prev, 0);
                return (K > 0 && routes > K) ? INF : total;
            }

            // ② Prins 最优拆分 DP。K=0:一维 d[j],弧 (i,j)=一车服务位置 i..j-1(需求 ≤ Q),O(N·B);
            //    K>0:加路数维 d[k][j]=前 j 客户用 k 路最小成本,答案 min_{k≤K} d[k][N],O(N·B·K)。
            double optimal(const std::vector<int>& nd) const
            {
                if (K <= 0)
                {
                    std::vector<double> d(N + 1, INF); d[0] = 0;
                    for (int i = 0; i < N; ++i)
                    {
                        if (d[i] >= INF) continue;
                        double load = 0, cost = 0;
                        for (int j = i + 1; j <= N; ++j)
                        {
                            int node = nd[j - 1]; load += dem[node];
                            if (load > Q + 1e-9) break;
                            if (j == i + 1) cost = C(0, node) + C(node, 0);
                            else { int pm = nd[j - 2]; cost += C(pm, node) + C(node, 0) - C(pm, 0); }
                            if (d[i] + cost < d[j]) d[j] = d[i] + cost;
                        }
                    }
                    return d[N];
                }
                std::vector<std::vector<double>> d(K + 1, std::vector<double>(N + 1, INF));
                d[0][0] = 0;
                for (int k = 1; k <= K; ++k)
                    for (int i = 0; i < N; ++i)
                    {
                        if (d[k - 1][i] >= INF) continue;
                        double load = 0, cost = 0;
                        for (int j = i + 1; j <= N; ++j)
                        {
                            int node = nd[j - 1]; load += dem[node];
                            if (load > Q + 1e-9) break;
                            if (j == i + 1) cost = C(0, node) + C(node, 0);
                            else { int pm = nd[j - 2]; cost += C(pm, node) + C(node, 0) - C(pm, 0); }
                            if (d[k - 1][i] + cost < d[k][j]) d[k][j] = d[k - 1][i] + cost;
                        }
                    }
                double best = INF; for (int k = 1; k <= K; ++k) best = std::min(best, d[k][N]);
                return best;
            }

            // ③ 线性拆分(Vidal 2016)：d[j]=g(j)+min_{i∈可行窗口} f(i),f(i)=d[i]+C(0,t_i)-ptour[i+1]
            //    只依赖 i、g(j)=ptour[j]+C(t_{j-1},0) 只依赖 j,容量窗口左界单调 → 滑窗最小值。
            //    K=0 单层 O(N);K>0 按路数分 K 层,每层对上一层做滑窗最小值,O(N·K)。
            double linear(const std::vector<int>& nd) const
            {
                std::vector<double> pdem(N + 1, 0), ptour(N + 1, 0);
                for (int i = 1; i <= N; ++i) pdem[i]  = pdem[i - 1]  + dem[nd[i - 1]];
                for (int i = 2; i <= N; ++i) ptour[i] = ptour[i - 1] + C(nd[i - 2], nd[i - 1]);
                auto g = [&](int j) { return ptour[j] + C(nd[j - 1], 0); };          // j ∈ 1..N
                if (K <= 0)
                {
                    std::vector<double> d(N + 1, INF); d[0] = 0;
                    auto f = [&](int i) { return d[i] + C(0, nd[i]) - ptour[i + 1]; };   // i ∈ 0..N-1
                    std::deque<int> dq; dq.push_back(0);
                    for (int j = 1; j <= N; ++j)
                    {
                        while (!dq.empty() && pdem[j] - pdem[dq.front()] > Q + 1e-9) dq.pop_front();
                        d[j] = dq.empty() ? INF : g(j) + f(dq.front());
                        if (j < N && d[j] < INF)
                        {
                            double fj = f(j);
                            while (!dq.empty() && f(dq.back()) >= fj) dq.pop_back();
                            dq.push_back(j);
                        }
                    }
                    return d[N];
                }
                std::vector<double> prev(N + 1, INF), cur(N + 1, INF); prev[0] = 0;
                double best = INF;
                for (int k = 1; k <= K; ++k)
                {
                    std::fill(cur.begin(), cur.end(), INF);
                    auto fk = [&](int i) { return prev[i] + C(0, nd[i]) - ptour[i + 1]; };   // 用上一层 prev
                    std::deque<int> dq;
                    for (int j = 1; j <= N; ++j)
                    {
                        if (prev[j - 1] < INF)                       // i=j-1 成为可用左端点
                        {
                            double fj = fk(j - 1);
                            while (!dq.empty() && fk(dq.back()) >= fj) dq.pop_back();
                            dq.push_back(j - 1);
                        }
                        while (!dq.empty() && pdem[j] - pdem[dq.front()] > Q + 1e-9) dq.pop_front();
                        if (!dq.empty()) cur[j] = g(j) + fk(dq.front());
                    }
                    best = std::min(best, cur[N]);
                    std::swap(prev, cur);
                }
                return best;
            }
            eccalcul_functor* copy() { return new routeFunc(*this); }
        };
        // 启发:最近邻(偏好离上一个客户最近的候选)。
        struct nnFunc : eccalcul_functor
        {
            int N; std::vector<double> x, y;
            nnFunc(int n, std::vector<double> xs, std::vector<double> ys) : N(n), x(std::move(xs)), y(std::move(ys)) {}
            double operator()(double** in) const
            {
                int pos = static_cast<int>(in[0][0] + 0.5);
                int cand = static_cast<int>(in[1][0] + 0.5) + 1;
                int prev = (pos == 0) ? 0 : static_cast<int>(in[2][pos - 1] + 0.5) + 1;
                return 1.0 / (1.0 + dist(x[prev], y[prev], x[cand], y[cand]));   // 越近分越高(非负,保序)
            }
            eccalcul_functor* copy() { return new nnFunc(*this); }
        };

    public:
        PT_CVRP() {}
        void setName(std::string name) { _name = name; }
        int  getProblemSize() { return _N; }
        void setPenalty(double p) { _penalty = p; }
        void setSplitMode(SplitMode m) { _split = m; }       // 切换切段策略(默认 Optimal)
        SplitMode getSplitMode() const { return _split; }
        void setVehicles(int k) { _K = k; }                  // 车辆数上限(0=不限,默认)
        int  getVehicles() const { return _K; }

        // 内存注入：N 客户;coords/dem 长度 N+1(下标 0=车场);容量 Q。
        void setData(int N, std::vector<double> xs, std::vector<double> ys, std::vector<double> dem, double Q)
        { _N = N; _x = std::move(xs); _y = std::move(ys); _dem = std::move(dem); _Q = Q; }

        void save(bool overwrite = false)
        {
            std::string path = "_pdata/cvrp/" + _name + ".cvrp";
            if (!overwrite) { std::ifstream ex(path); if (ex.good()) { sys_logger.error("CVRP save: file exists (use overwrite): " + path); return; } }
            std::ofstream out(path);
            if (!out) { sys_logger.error("CVRP save: cannot write " + path); return; }
            out << "NAME: " << _name << "\nTYPE: CVRP\nDIMENSION: " << (_N + 1) << "\nCAPACITY: " << _Q << "\n";
            if (_K > 0) out << "VEHICLES: " << _K << "\n";
            out << "NODE_COORD_SECTION\n"; for (int i = 0; i <= _N; ++i) out << (i + 1) << " " << _x[i] << " " << _y[i] << "\n";  // 节点 1=车场
            out << "DEMAND_SECTION\n";     for (int i = 0; i <= _N; ++i) out << (i + 1) << " " << _dem[i] << "\n";
        }

        Problem* getProblem()
        {
            if (_N == 0) return nullptr;
            Problem* back = new Problem(_name);
            back->addVariable("x", 0, _N - 1, 1, _N, 1, VariableType::sequence_bidiagraph);   // 客户排列
            double pen = (_penalty > 0) ? _penalty : 1e6;
            routeFunc rf(_N, _Q, _K, _x, _y, _dem, _split);
            back->addObjective("dist", 1, true, "x", &rf);                                    // 最小化总距离(K 受限划分)
            back->addConstrainUnique("x", pen, "dist");   // 每客户恰一次:constrains_variable(构造期删已用值)+ violation 兜底
            nnFunc nf(_N, _x, _y);
            back->addInspirationFunc("x", "x", &nf);                                           // 最近邻
            return back;
        }

        void load(std::string name)
        {
            setName(instanceName(name));
            std::ifstream f(resolveInstancePath(name, "cvrp", "cvrp"));
            if (!f) { sys_logger.error("CVRP instance not found: " + name); return; }
            int dim = 0, K = 0; double Q = 0; std::string tok;
            std::vector<double> xs, ys, dem;
            while (f >> tok)
            {
                if (tok == "DIMENSION:") f >> dim;
                else if (tok == "CAPACITY:") f >> Q;
                else if (tok == "VEHICLES:") f >> K;
                else if (tok == "NODE_COORD_SECTION")
                { xs.assign(dim, 0); ys.assign(dim, 0); for (int i = 0; i < dim; ++i) { int id; double a, b; f >> id >> a >> b; xs[id - 1] = a; ys[id - 1] = b; } }
                else if (tok == "DEMAND_SECTION")
                { dem.assign(dim, 0); for (int i = 0; i < dim; ++i) { int id; double d; f >> id >> d; dem[id - 1] = d; } }
                else if (!tok.empty() && tok.back() == ':') { std::string rest; std::getline(f, rest); }
            }
            setData(dim - 1, xs, ys, dem, Q);   // 客户数 = dim-1(去车场)
            setVehicles(K);                     // VEHICLES 可选(缺省 0=不限)
        }
    };
}
