//------------------------Description------------------------
// 领袖+随机学习拓扑 LeaderAndRandom:每个体获得两个学习对象——end[0]=领袖(全局最优)、end[1]=一个随机个体。
//   为 WOA(领袖收缩/螺旋 + 随机鲸探索)等"最优引导 + 随机探索"类策略提供数据;通用可复用(如 DE current-to-best/rand)。
//-------------------------Reference-------------------------
// 无特定文献:通用"最优 + 随机"两学习对象拓扑,供 WOA(WhaleForaging)等复用。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "best-archive.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class LeaderAndRandom : public LearningTopology
    {
    public:
        LeaderAndRandom() : LearningTopology() {}
        ~LeaderAndRandom() {}

        static void preAssert(AssertList& list, double* paras) {}   // 无个体类型要求

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));      // end[0]=领袖, end[1]=随机
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 起点=当前种群,规模=100%
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int N = cswarm->getSize();

            // 领袖=全局最优(遵 WOA 原文):档案非空取 getElite();档案尚空(首代)回退当代种群最优。
            Solution* leader = nullptr;
            if (best_holder && best_holder[0] && best_holder[0]->getBestSize() > 0)
                leader = best_holder[0]->getElite();
            else if (N > 0)
            {
                int best = 0;
                for (int i = 1; i < N; i++)
                    if ((*cswarm)[i] < (*cswarm)[best]) best = i;
                leader = &(*cswarm)[best].solution;
            }

            LearningGraph* graph = new LearningGraph(N, 2);
            for (int i = 0; i < N; i++)
            {
                graph->addStart(&(*cswarm)[i]);
                graph->addEnd(leader);                                                // end[0] = 领袖鲸(全局最优)
                graph->addEnd(&(*cswarm)[ECFlow::get_int(0, N - 1)].solution);          // end[1] = 随机鲸(每个体各取)
            }
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry leaderAndRandomEntry()
    {
        return { "LeaderAndRandom", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(LeaderAndRandom),
            [](const double*) -> LearningTopology* { return new LeaderAndRandom(); },
            [](AssertList& L, const double* p) { LeaderAndRandom::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { LeaderAndRandom::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_leaderrandom, LearningTopology, leaderAndRandomEntry());
}
