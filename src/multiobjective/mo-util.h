//------------------------Description------------------------
// 多目标基础工具 MOUtil:非支配排序(fastNonDominatedSort)+ 拥挤度距离(crowdingDistance)。供 NSGA-II 环境选择、
//-------------------------Reference-------------------------
// K. Deb, A. Pratap, S. Agarwal, T. Meyarivan, "A fast and elitist multiobjective genetic algorithm: NSGA-II,"
// IEEE Trans. Evolutionary Computation, vol. 6, no. 2, pp. 182-197, 2002, doi: 10.1109/4235.996017.
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
#include <cmath>
#include <utility>
#include "individual.h"
#include "ecflow-constant.h"

namespace ECFlow
{
    class MOUtil
    {
    public:
        // 非支配排序:rank_back[i] = 个体 i 的支配前沿层(1 = 第一非支配前沿 = 最优)。O(N²·M)。
        static void fastNonDominatedSort(Individual** swarm, int ss, double* rank_back)
        {
            std::vector<int> n(ss, 0);                 // 被支配计数
            std::vector<std::vector<int>> S(ss);       // 各个体支配的集合
            std::vector<int> F;                        // 当前前沿

            for (int i = 0; i < ss; i++)
            {
                for (int j = 0; j < ss; j++)
                {
                    if (i == j) continue;
                    if ((*swarm[i]) < (*swarm[j])) S[i].push_back(j);      // i 支配 j
                    else if ((*swarm[j]) < (*swarm[i])) n[i]++;            // j 支配 i
                }
                if (n[i] == 0) { rank_back[i] = 1; F.push_back(i); }
            }

            int ra = 1;
            while (!F.empty())
            {
                std::vector<int> Q;
                for (int fi : F)
                    for (int j : S[fi])
                        if (--n[j] == 0) { rank_back[j] = ra + 1; Q.push_back(j); }
                ra++;
                F = std::move(Q);
            }
        }

        // 拥挤度距离:distance_back[i] 越大越稀疏(应优先保留);边界点(任一目标极值)得巨大值必留。
        //   normalize=true 时按各目标量程归一(混合尺度目标更稳)。拥挤度须在**同一前沿内**计算(NSGA-II 语义)。
        static void crowdingDistance(Individual** swarm, int ss, double* distance_back, bool normalize = false)
        {
            for (int i = 0; i < ss; i++) distance_back[i] = 0.0;
            if (ss <= 0) return;

            int obj_n = swarm[0]->getObjectNumber();
            if (ss <= 2) { for (int i = 0; i < ss; i++) distance_back[i] = double(ECFLOW_MAX); return; }  // 全为边界

            std::vector<int> idx(ss);
            for (int o = 0; o < obj_n; o++)
            {
                for (int i = 0; i < ss; i++) idx[i] = i;
                std::sort(idx.begin(), idx.end(),
                          [&swarm, o](int a, int b) { return swarm[a]->solution.fitness[o] < swarm[b]->solution.fitness[o]; });

                double range = swarm[idx[ss - 1]]->solution.fitness[o] - swarm[idx[0]]->solution.fitness[o];
                distance_back[idx[0]]      += double(ECFLOW_MAX);   // 该目标极小/极大 = 边界,必留
                distance_back[idx[ss - 1]] += double(ECFLOW_MAX);
                if (normalize && range <= 1e-12) continue;         // 该目标全等 → 内部不贡献(避免除零)

                for (int i = 1; i < ss - 1; i++)
                {
                    double gap = swarm[idx[i + 1]]->solution.fitness[o] - swarm[idx[i - 1]]->solution.fitness[o];
                    distance_back[idx[i]] += normalize ? gap / range : gap;
                }
            }
        }

        // ===== MOEA/D 分解式工具 =====

        // 确定式散列(splitmix64 变体):给权重生成用,同输入恒同输出(拓扑/选择器一致)。
        static unsigned long long detHash(unsigned long long x)
        {
            x += 0x9E3779B97F4A7C15ull;
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
            return x ^ (x >> 31);
        }

        // 生成恰好 N 个权重向量(单纯形上)。**确定**(同 N,m 每次相同)。
        //   m=2:精确均匀 λⁱ=(i/(N-1), 1-i/(N-1));m≥3:确定式散列单纯形采样(任意 N,可用;格点升级见 future)。
        static std::vector<std::vector<double>> generateWeights(int N, int m)
        {
            std::vector<std::vector<double>> W(N, std::vector<double>(m, 0.0));
            if (N <= 0) return W;
            if (m == 2)
            {
                for (int i = 0; i < N; i++)
                {
                    double t = (N == 1) ? 0.0 : double(i) / (N - 1);
                    W[i][0] = t; W[i][1] = 1.0 - t;
                }
            }
            else
            {
                for (int i = 0; i < N; i++)
                {
                    double s = 0.0;
                    for (int j = 0; j < m; j++)
                    {
                        unsigned long long h = detHash((unsigned long long)i * 131ull + (unsigned long long)j * 977ull + 12345ull);
                        double u = (double(h % 1000000ull) + 1.0) / 1000001.0;   // ∈ (0,1)
                        W[i][j] = -std::log(u);
                        s += W[i][j];
                    }
                    if (s <= 0.0) s = 1.0;
                    for (int j = 0; j < m; j++) W[i][j] /= s;
                }
            }
            return W;
        }

        // 每个权重的 T 个最近邻(欧氏距离,含自身)。m-无关,任意 m 鲁棒。
        static std::vector<std::vector<int>> neighborhoods(const std::vector<std::vector<double>>& W, int T)
        {
            int N = (int)W.size();
            int m = N ? (int)W[0].size() : 0;
            if (T > N) T = N;
            if (T < 1) T = 1;
            std::vector<std::vector<int>> B(N);
            std::vector<std::pair<double, int>> d(N);
            for (int i = 0; i < N; i++)
            {
                for (int k = 0; k < N; k++)
                {
                    double s = 0.0;
                    for (int j = 0; j < m; j++) { double diff = W[i][j] - W[k][j]; s += diff * diff; }
                    d[k] = std::make_pair(s, k);
                }
                std::partial_sort(d.begin(), d.begin() + T, d.end());
                B[i].resize(T);
                for (int t = 0; t < T; t++) B[i][t] = d[t].second;
            }
            return B;
        }

        // Tchebycheff 标量化(全最小化:f_j≥z_j;λ=0 用 eps 防忽略该目标)。g = max_j λ_j·(f_j - z_j)。
        static double tchebycheff(const double* f, const std::vector<double>& w, const std::vector<double>& z, int m)
        {
            double g = 0.0;
            for (int j = 0; j < m; j++)
            {
                double lam = (w[j] < 1e-6) ? 1e-6 : w[j];
                double v = lam * (f[j] - z[j]);
                if (v > g) g = v;
            }
            return g;
        }
    };
}
