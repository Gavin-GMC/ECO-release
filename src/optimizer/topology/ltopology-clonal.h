//------------------------Description------------------------
// 免疫算法用两个拓扑:
//   ClonalExpansion —— 克隆扩增:按亲和度(适应度排名)给好抗体发更多起点边(N_c(r)=round(β·N/r)),克隆数∝亲和度。
//   RandomSelect    —— 随机选 d 个个体作起点(用于感受器编辑:多做一次生成产 d 个新解)。
//-------------------------Reference-------------------------
// L. N. de Castro, F. J. Von Zuben, "Learning and optimization using the clonal selection principle,"
// IEEE Trans. Evolutionary Computation, vol. 6, no. 3, pp. 239-251, 2002, doi: 10.1109/TEVC.2002.1011539.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 克隆扩增:好抗体克隆更多(N_c ∝ 亲和度排名)
    class ClonalExpansion : public LearningTopology
    {
    private:
        double _clone_factor;   // β:克隆规模因子

    public:
        explicit ClonalExpansion(double clone_factor = 1.0) : LearningTopology(), _clone_factor(clone_factor) {}
        ~ClonalExpansion() {}

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::postAssert)); // 无学习对象(自变异)
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();

            // 按亲和度排名(idx[0]=最优)
            std::vector<int> idx(N);
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(),
                      [cswarm](int a, int b) { return (*cswarm)[a] < (*cswarm)[b]; });

            // 克隆数 N_c(r)=max(1,round(β·N/rank)),rank 从 1 起
            std::vector<int> nc(N, 1);
            long long total = 0;
            for (int r = 0; r < N; r++)
            {
                int c = (int)std::lround(_clone_factor * (double)N / (double)(r + 1));
                if (c < 1) c = 1;
                nc[idx[r]] = c;
                total += c;
            }

            LearningGraph* graph = new LearningGraph((size_t)total, 0);
            for (int r = 0; r < N; r++)
            {
                int ai = idx[r];
                for (int j = 0; j < nc[ai]; j++)
                    graph->addStart(&(*cswarm)[ai]);   // 同一抗体重复作起点 = 克隆
            }
            return graph;
        }
    };

    // 随机选 d 个个体作起点(感受器编辑:产 d 个随机新生)
    class RandomSelect : public LearningTopology
    {
    private:
        int _count;   // d:选取数目

    public:
        explicit RandomSelect(int count = 1) : LearningTopology(), _count(count) {}
        ~RandomSelect() {}

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::postAssert)); // 无学习对象(随机生成)
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();
            int d = (_count < N) ? _count : N;   // 不超过种群规模
            if (d < 0) d = 0;

            LearningGraph* graph = new LearningGraph((size_t)d, 0);
            for (int i = 0; i < d; i++)
                graph->addStart(&(*cswarm)[(N > 0) ? ECFlow::get_int(0, N - 1) : 0]);   // 随机成员(取谁无关:随机生成忽略父代)
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry clonalExpansionEntry()
    {
        return { "ClonalExpansion", ModuleType::T_learntopology,
            ParameterTemplate{ { {"clone_factor", ParamKind::Real, 0.0, 100.0, false, 0.5, 2.0} } }, sizeof(ClonalExpansion),
            [](const double* p) -> LearningTopology* { return new ClonalExpansion(p ? p[0] : 1.0); },
            [](AssertList& L, const double* p) { ClonalExpansion::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ClonalExpansion::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry randomSelectEntry()
    {
        return { "RandomSelect", ModuleType::T_learntopology,
            ParameterTemplate{ { {"count", ParamKind::Int, 0, 0x3f3f3f3f, false, 1, 10} } }, sizeof(RandomSelect),
            [](const double* p) -> LearningTopology* { return new RandomSelect(p ? (int)p[0] : 1); },
            [](AssertList& L, const double* p) { RandomSelect::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RandomSelect::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_clonalexpansion, LearningTopology, clonalExpansionEntry());
    ECFLOW_REGISTER(ltopo_randomselect,    LearningTopology, randomSelectEntry());
}
