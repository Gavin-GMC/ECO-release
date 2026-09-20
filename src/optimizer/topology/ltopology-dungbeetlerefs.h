//------------------------Description------------------------
// 蜣螂参照学习拓扑 DungBeetleRefs:每个体给 3 个学习对象——
//   end[0]=全局最优 X^b(档案 best-so-far)、end[1]=当代最优 X*(local best)、end[2]=当代最差 X_worst。
//   供 DBO(DungBeetleRoles)四角色更新;通用可复用。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "learning-topology.h"
#include "best-archive.h"
#include "registry.h"

namespace EvoFlow
{
    class DungBeetleRefs : public LearningTopology
    {
    public:
        DungBeetleRefs() : LearningTopology() {}
        ~DungBeetleRefs() {}

        static void preAssert(AssertList&, double*) {}
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::postAssert));      // 全局最优/当代最优/当代最差
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }
        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int, IndividualArray*, LearningGraph*) override
        {
            IndividualArray* cs = subswarm[0];
            int N = cs->getSize();
            LearningGraph* g = new LearningGraph(N, 3);
            if (N == 0) return g;

            int ib = 0, iw = 0;
            for (int i = 1; i < N; i++)
            {
                if ((*cs)[i] < (*cs)[ib]) ib = i;   // 当代最优
                if ((*cs)[iw] < (*cs)[i]) iw = i;   // 当代最差
            }
            Solution* localbest = &(*cs)[ib].solution;
            Solution* worst = &(*cs)[iw].solution;
            Solution* globalbest = (best_holder && best_holder[0] && best_holder[0]->getBestSize() > 0)
                                 ? best_holder[0]->getElite() : localbest;

            for (int i = 0; i < N; i++)
            {
                g->addStart(&(*cs)[i]);
                g->addEnd(globalbest);   // end[0] = 全局最优 X^b
                g->addEnd(localbest);    // end[1] = 当代最优 X*
                g->addEnd(worst);        // end[2] = 当代最差 X_worst
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry dungBeetleRefsEntry()
    {
        return { "DungBeetleRefs", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(DungBeetleRefs),
            [](const double*) -> LearningTopology* { return new DungBeetleRefs(); },
            [](AssertList& L, const double* p) { DungBeetleRefs::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DungBeetleRefs::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_dungbeetlerefs, LearningTopology, dungBeetleRefsEntry());
}
