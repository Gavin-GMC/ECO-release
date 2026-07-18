//------------------------Description------------------------
// 孤立拓扑 IsolateTopology:个体不向任何特定个体学习(每点 0 学习对象),用于独立构造类算法(如 ACO)。
//-------------------------Reference-------------------------
// 无特定文献:个体不设学习对象、独立构造(如 ACO 的独立解构建);框架基础拓扑。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "registry.h"

namespace ECFlow
{
    class IsolateTopology : public LearningTopology
    {
    public:
        IsolateTopology() : LearningTopology() {}
        ~IsolateTopology() {}

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 输出图规模 = 100% 当前种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 0);

            Individual* s1;
            for (int i = 0; i < graph_size; i++)
            {
                s1 = &cswarm[0][i];
                back->addStart(s1);
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry isolateTopologyEntry()
    {
        return { "Isolate", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(IsolateTopology),
            [](const double*) -> LearningTopology* { return new IsolateTopology(); },
            [](AssertList& L, const double* p) { IsolateTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { IsolateTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_isolate, LearningTopology, isolateTopologyEntry());
}
