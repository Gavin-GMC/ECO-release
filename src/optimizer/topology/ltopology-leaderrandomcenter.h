//------------------------Description------------------------
// 领袖+随机+中心学习拓扑 LeaderRandomCenter:每个体获得三个学习对象——
//   end[0]=领袖(全局最优)、end[1]=一个随机个体、end[2]=**逐维种群均值**(群体中心)。
//   为 RSA(ReptileEncircling)等"最优引导 + 随机探索 + 群体中心偏离"类策略提供数据;通用可复用。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "learning-topology.h"
#include "best-archive.h"
#include "evoflow-rand.h"
#include "evoflow-basicfunc.h"
#include "registry.h"

namespace EvoFlow
{
    class LeaderRandomCenter : public LearningTopology
    {
    private:
        Solution population_mean;

        // 逐维种群均值(与 CompetitionTopology::cal_mean 同款):population_mean[i] = 全群第 i 维平均。
        void cal_mean(IndividualArray& population)
        {
            population_mean.setSize(population[0].solution);
            size_t p_size = population_mean.getSolutionSize();
            size_t s_size = population.getSize();
            for (size_t i = 0; i < p_size; i++)
            {
                double buffer = 0.0;
                size_t valid_count = 0;
                for (size_t j = 0; j < s_size; j++)
                {
                    double value = population[j][i];
                    if (is_empty(value)) continue;
                    buffer += value;
                    valid_count++;
                }
                population_mean[i] = valid_count > 0 ? buffer / valid_count : EMPTYVALUE;
            }
        }

    public:
        LeaderRandomCenter() : LearningTopology() {}
        ~LeaderRandomCenter() {}

        static void preAssert(AssertList&, double*) {}   // 无个体类型要求

        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::postAssert));      // end[0]=最优, end[1]=随机, end[2]=种群均值
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

            cal_mean(*cswarm);   // 逐维种群均值

            for (int i = 0; i < N; i++)
            {
                graph->addStart(&(*cswarm)[i]);
                graph->addEnd(leader);                                              // end[0] = 全局最优
                graph->addEnd(&(*cswarm)[EvoFlow::get_int(0, N - 1)].solution);        // end[1] = 随机成员(每个体各取)
                graph->addEnd(&population_mean);                                     // end[2] = 逐维种群均值(共享)
            }
            return graph;
        }
    };

    inline Registry<LearningTopology>::Entry leaderRandomCenterEntry()
    {
        return { "LeaderRandomCenter", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(LeaderRandomCenter),
            [](const double*) -> LearningTopology* { return new LeaderRandomCenter(); },
            [](AssertList& L, const double* p) { LeaderRandomCenter::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { LeaderRandomCenter::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_leaderrandomcenter, LearningTopology, leaderRandomCenterEntry());
}
