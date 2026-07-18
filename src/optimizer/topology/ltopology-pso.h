//------------------------Description------------------------
// PSO 学习拓扑 PGBestTopology:每个个体向 gbest(全局最优档案)与自身 pbest 学习。
//-------------------------Reference-------------------------
// [1] J. Kennedy and R. Eberhart, "Particle Swarm Optimization," in Proc. ICNN'95 - Int. Conf. Neural
//     Networks, Perth, WA, Australia, 1995, vol. 4, pp. 1942-1948, doi: 10.1109/ICNN.1995.488968.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "individual.h"       // getTopology 用 feature<SolutionFeature>(pbest)
#include "registry.h"

namespace ECFlow
{
    class PGBestTopology : public LearningTopology
    {
    public:
        PGBestTopology() : LearningTopology() {}
        ~PGBestTopology() {}

        // pbest = 每个体唯一的历史最优(Singular/solution),所有读者共用
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "pbest", "pbest", FeatureScope::Singular, {} } };
        }

        static void preAssert(AssertList& list, double* paras)
        {
            // pbest 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }

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
            BestArchive* cholder = best_holder[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 2);
            std::string pbest_key = featureKey("pbest");

            for (int i = 0; i < graph_size; i++)
            {
                Individual* ind = const_cast<Individual*>(&cswarm[0][i]);
                back->addStart(ind);
                back->addEnd(cholder->getBest());
                back->addEnd(&ind->feature<SolutionFeature>(pbest_key)->sol);   // 自身 pbest 特性
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry psoTopologyEntry()
    {
        return { "PGBest", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(PGBestTopology),
            [](const double*) -> LearningTopology* { return new PGBestTopology(); },
            [](AssertList& L, const double* p) { PGBestTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { PGBestTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_pgbest, LearningTopology, psoTopologyEntry());
}
