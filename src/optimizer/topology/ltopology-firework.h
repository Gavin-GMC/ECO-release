//------------------------Description------------------------
// 烟花爆炸拓扑 FireworkExplosion:每个烟花按适应度好坏爆炸出不同数目的火花(好烟花产更多火花)。
//   火花数 S_i ∝ (烟花 i 比最差好多少),截断到 [a·M, b·M];起点=烟花(重复 S_i 次),无学习对象(自扰动)。
//-------------------------Reference-------------------------
// Y. Tan, Y. Zhu, "Fireworks Algorithm for Optimization," ICSI 2010, LNCS 6145, pp. 355-364,
// doi: 10.1007/978-3-642-13495-1_44.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <cmath>
#include "learning-topology.h"
#include "registry.h"

namespace ECFlow
{
    class FireworkExplosion : public LearningTopology
    {
    private:
        double _M;   // 火花总量基数
        double _a;   // 火花数下界比例(0<a<b<1)
        double _b;   // 火花数上界比例

    public:
        FireworkExplosion(double M = 40.0, double a = 0.04, double b = 0.8)
            : LearningTopology(), _M(M), _a(a), _b(b) {}
        ~FireworkExplosion() {}

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::postAssert)); // 无学习对象(自扰动)
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();
            if (N <= 0) return new LearningGraph(0, 0);

            // best/worst(方向感知),取 fitness 值
            int best = 0, worst = 0;
            for (int i = 1; i < N; i++)
            {
                if ((*cswarm)[i] < (*cswarm)[best]) best = i;
                if ((*cswarm)[worst] < (*cswarm)[i]) worst = i;   // worst 比 i 差 → i 更好 → worst 保持最差
            }
            double f_worst = (*cswarm)[worst].solution.fitness[0];

            const double eps = 1e-12;
            std::vector<double> quality(N);
            double sum_q = 0.0;
            for (int i = 0; i < N; i++)
            {
                quality[i] = std::fabs((*cswarm)[i].solution.fitness[0] - f_worst);   // 离最差越远越好
                sum_q += quality[i];
            }

            int lo = (int)std::lround(_a * _M);
            int hi = (int)std::lround(_b * _M);
            if (lo < 1) lo = 1;
            if (hi < lo) hi = lo;

            std::vector<int> nc(N);
            long long total = 0;
            for (int i = 0; i < N; i++)
            {
                double frac = (quality[i] + eps) / (sum_q + eps);
                int c = (int)std::lround(_M * frac);
                if (c < lo) c = lo;
                if (c > hi) c = hi;
                nc[i] = c;
                total += c;
            }

            LearningGraph* graph = new LearningGraph((size_t)total, 0);
            for (int i = 0; i < N; i++)
                for (int j = 0; j < nc[i]; j++)
                    graph->addStart(&(*cswarm)[i]);   // 烟花 i 重复 S_i 次 = 它的火花起点
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry fireworkExplosionEntry()
    {
        return { "FireworkExplosion", ModuleType::T_learntopology,
            ParameterTemplate{ { {"M", ParamKind::Real, 1.0, 10000.0, false, 20.0, 50.0},
                                 {"a", ParamKind::Real, 0.0, 1.0, false, 0.02, 0.1},
                                 {"b", ParamKind::Real, 0.0, 1.0, false, 0.6, 0.9} } }, sizeof(FireworkExplosion),
            [](const double* p) -> LearningTopology* { return p ? new FireworkExplosion(p[0], p[1], p[2]) : new FireworkExplosion(); },
            [](AssertList& L, const double* p) { FireworkExplosion::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { FireworkExplosion::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_fireworkexplosion, LearningTopology, fireworkExplosionEntry());
}
