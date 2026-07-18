//------------------------Description------------------------
// 随机支配学习拓扑 StochasticDominantLearningTopology:每个个体从种群随机选两个个体,排序后仅当自身
//   劣于其中较优者时才向这两个(支配它的)个体学习,否则不学习。
//-------------------------Reference-------------------------
// [1] Q. Yang, W.-N. Chen, T. Gu, H. Jin, W. Mao and J. Zhang, "An Adaptive Stochastic Dominant
//     Learning Swarm Optimizer for High-Dimensional Optimization," IEEE Transactions on Cybernetics,
//     vol. 52, no. 3, pp. 1960-1976, March 2022, doi: 10.1109/TCYB.2020.3034427.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <utility>
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class StochasticDominantLearningTopology : public LearningTopology
    {
    public:
        StochasticDominantLearningTopology() : LearningTopology() {}
        ~StochasticDominantLearningTopology() {}

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 输出图规模 = 100% 当前种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 2);

            int sid1, sid2;

            for (int sid = 0; sid < graph_size; sid++)
            {
                // 插入起始点
                back->addStart(&cswarm[0][sid]);

                // 随机选择两个个体
                sid1 = ECFlow::get_int(0, graph_size - 1);
                sid2 = ECFlow::get_int(0, graph_size - 1);

                // 根据优劣对两个对象进行排序(sid1 更优)
                if (cswarm[0][sid2] < cswarm[0][sid1])
                {
                    std::swap(sid1, sid2);
                }
                // 判断与当前个体的优劣关系
                if (cswarm[0][sid] < cswarm[0][sid2])
                {
                    // 当前个体优于较优候选 → 放弃学习(契约:end[0]=null → 直接进子代)
                    back->addEnd(nullptr);
                }
                else
                {
                    // 向两个支配个体学习
                    back->addEnd(&cswarm[0][sid1].solution);
                    back->addEnd(&cswarm[0][sid2].solution);
                }
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry stochasticDominantLearningTopologyEntry()
    {
        return { "StochasticDominantLearning", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(StochasticDominantLearningTopology),
            [](const double*) -> LearningTopology* { return new StochasticDominantLearningTopology(); },
            [](AssertList& L, const double* p) { StochasticDominantLearningTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { StochasticDominantLearningTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_stochasticdominant, LearningTopology, stochasticDominantLearningTopologyEntry());
}
