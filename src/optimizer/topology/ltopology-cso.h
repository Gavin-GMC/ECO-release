//------------------------Description------------------------
// 竞争学习拓扑 CompetitionTopology:随机两两配对竞争,败者向胜者 + 种群均值学习,胜者不学习。

//-------------------------Reference-------------------------
// [1] R. Cheng and Y. Jin, "A Competitive Swarm Optimizer for Large Scale Optimization," IEEE
//     Transactions on Cybernetics, vol. 45, no. 2, pp. 191-204, Feb. 2015, doi: 10.1109/TCYB.2014.2322602.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <utility>
#include "learning-topology.h"
#include "evoflow-rand.h"
#include "evoflow-basicfunc.h"
#include "registry.h"

namespace EvoFlow
{
    class CompetitionTopology : public LearningTopology
    {
    private:
        int* id_list;
        int id_list_size;
        Solution population_mean;

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
        CompetitionTopology() : LearningTopology()
        {
            id_list = nullptr;
            id_list_size = 0;
        }

        ~CompetitionTopology() { delete[] id_list; }   // 迁移修复:原空析构 → 泄漏

        static void preAssert(AssertList& list, double* paras) {}

        static void postAssert(AssertList& list, double* paras)
        {
            // v2.3.1.4 修正:原声明 1,**与实现不符** —— 败者路径 addEnd(winner) + addEnd(population_mean)
            //   实为 **2** 个学习对象(见 update 的"败者:向胜者 + 种群均值学习",亦合 CSO 原文)。
            //   该错误长期潜伏未被发现,因 AssertMatcher 的结果缓存以 wf.tag 为键、而 config/ 下
            //   97 份配置的 tag 全是 `wf` ⇒ 批量校验时只有第一份被真验、其余全命中缓存
            //   (缓存已于本版移除)。`CSO.cfg` 写 object_number=2 是对的。
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "coupled", 2, MatchType::postAssert)); // 两个个体间相互学习
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 输出图规模 = 100% 当前种群
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            LearningGraph* back = new LearningGraph(graph_size, 2);

            cal_mean(*cswarm);

            if (id_list_size != graph_size)
            {
                delete[] id_list;
                id_list = new int[graph_size];
                for (int i = 0; i < graph_size; i++)
                    id_list[i] = i;
                id_list_size = graph_size;   // 迁移修复:回写尺寸,守卫生效(避免每次重分配)
            }

            shuffle(id_list, graph_size);    // EvoFlow::shuffle(evoflow-rand.h),过 EvoFlow 引擎

            int winner, loser;
            Individual* s1;
            Solution* s2;
            for (int i = 0; i < graph_size; i++)
            {
                if (i == graph_size - 1)
                {
                    s1 = &cswarm[0][id_list[i]];
                    back->addStart(s1);
                    back->addEnd(nullptr); // 无学习对象(契约:end[0]=null → 直接进子代)
                    continue;
                }
                winner = id_list[i];
                loser = id_list[i + 1];

                if (cswarm[0][loser] < cswarm[0][winner])
                    std::swap(loser, winner);

                // 胜者:不学习
                s1 = &cswarm[0][winner];
                back->addStart(s1);
                back->addEnd(nullptr);

                // 败者:向胜者 + 种群均值学习
                s1 = &cswarm[0][loser];
                s2 = &cswarm[0][winner].solution;
                back->addStart(s1);
                back->addEnd(s2);
                back->addEnd(&population_mean);
                i++;
            }

            return back;
        }
    };

    inline Registry<LearningTopology>::Entry competitionTopologyEntry()
    {
        return { "Competition", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(CompetitionTopology),
            [](const double*) -> LearningTopology* { return new CompetitionTopology(); },
            [](AssertList& L, const double* p) { CompetitionTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { CompetitionTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_competition, LearningTopology, competitionTopologyEntry());
}
