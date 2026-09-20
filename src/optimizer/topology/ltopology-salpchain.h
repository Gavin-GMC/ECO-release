//------------------------Description------------------------
// 海鞘链学习拓扑 SalpChain:按种群 index 顺序成链——每 salp i 给 2 个学习对象:
//   end[0]=食物源 F(全局最优);end[1]=**链上前一只 salp[i-1] 的父代位置**(i==0 领导则为空)。
//   供 SalpChainUpdate(SSA):领导跟食物、跟随者与前一只平均。**并行版**(end[1] 取父代当前位置)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "learning-topology.h"
#include "best-archive.h"
#include "registry.h"

namespace EvoFlow
{
    class SalpChain : public LearningTopology
    {
    public:
        SalpChain() : LearningTopology() {}
        ~SalpChain() {}

        static void preAssert(AssertList&, double*) {}

        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));      // end[0]=食物, end[1]=前一只(领导为空)
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int, IndividualArray*, LearningGraph*) override
        {
            IndividualArray* cs = subswarm[0];
            int N = cs->getSize();
            LearningGraph* g = new LearningGraph(N, 2);
            if (N == 0) return g;

            Solution* food = nullptr;
            if (best_holder && best_holder[0] && best_holder[0]->getBestSize() > 0)
                food = best_holder[0]->getElite();
            else
            {
                int b = 0;
                for (int i = 1; i < N; i++) if ((*cs)[i] < (*cs)[b]) b = i;
                food = &(*cs)[b].solution;
            }

            for (int i = 0; i < N; i++)
            {
                g->addStart(&(*cs)[i]);
                g->addEnd(food);                                                   // end[0] = 食物源
                g->addEnd(i > 0 ? &(*cs)[i - 1].solution : nullptr);               // end[1] = 前一只(领导 i==0 为空)
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry salpChainEntry()
    {
        return { "SalpChain", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(SalpChain),
            [](const double*) -> LearningTopology* { return new SalpChain(); },
            [](AssertList& L, const double* p) { SalpChain::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SalpChain::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_salpchain, LearningTopology, salpChainEntry());
}
