//------------------------Description------------------------
// 领袖+双随机学习拓扑 LeaderTwoRandom:每个体获得三个学习对象——end[0]=领袖(全局最优)、
//   end[1]/end[2]=两个随机个体。为 MPA(MarinePredatorMotion)的 FADs 双随机猎物差(Prey_r1−Prey_r2)等
//   "最优引导 + 差分扰动"类策略提供数据;通用可复用(如 DE rand/2 的两随机项)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "learning-topology.h"
#include "best-archive.h"
#include "evoflow-rand.h"
#include "registry.h"

namespace EvoFlow
{
    class LeaderTwoRandom : public LearningTopology
    {
    public:
        LeaderTwoRandom() : LearningTopology() {}
        ~LeaderTwoRandom() {}

        static void preAssert(AssertList&, double*) {}   // 无个体类型要求

        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::postAssert));      // end[0]=领袖, end[1]/[2]=两随机
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 起点=当前种群,规模=100%
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();
            LearningGraph* graph = new LearningGraph(N, 3);
            if (N == 0) return graph;

            // 领袖=全局最优:档案非空取 getElite();否则回退当代种群最优。
            Solution* leader = nullptr;
            if (best_holder && best_holder[0] && best_holder[0]->getBestSize() > 0)
                leader = best_holder[0]->getElite();
            else
            {
                int best = 0;
                for (int i = 1; i < N; i++)
                    if ((*cswarm)[i] < (*cswarm)[best]) best = i;
                leader = &(*cswarm)[best].solution;
            }

            for (int i = 0; i < N; i++)
            {
                graph->addStart(&(*cswarm)[i]);
                graph->addEnd(leader);                                              // end[0] = 全局最优
                graph->addEnd(&(*cswarm)[EvoFlow::get_int(0, N - 1)].solution);        // end[1] = 随机猎物 1
                graph->addEnd(&(*cswarm)[EvoFlow::get_int(0, N - 1)].solution);        // end[2] = 随机猎物 2
            }
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry leaderTwoRandomEntry()
    {
        return { "LeaderTwoRandom", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(LeaderTwoRandom),
            [](const double*) -> LearningTopology* { return new LeaderTwoRandom(); },
            [](AssertList& L, const double* p) { LeaderTwoRandom::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { LeaderTwoRandom::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_leadertworandom, LearningTopology, leaderTwoRandomEntry());
}
