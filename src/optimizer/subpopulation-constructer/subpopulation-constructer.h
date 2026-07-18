//------------------------Description------------------------
// SubpopulationConstructer:子种群"构建/重构"策略基类——把全体个体(重新)划分到各子种群。
//   ini() 默认即 build();由 SubpopulationManager 在 ini/update 时调用。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "subpopulation.h"
#include "parameter-template.h"
#include "registry.h"
#include "ecflow-math.h"        // eu_distance
#include "ecflow-constant.h"    // ECFLOW_MAX

namespace ECFlow
{
    class SubpopulationConstructer
    {
    public:
        SubpopulationConstructer() {}
        virtual ~SubpopulationConstructer() {}

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        virtual void build(Subpopulation** subpopulations, int& swarm_number) = 0;

        virtual void ini(Subpopulation** subpopulations, int& swarm_number)
        {
            build(subpopulations, swarm_number);
        }
    };

    // 固定:不重构子群(no-op),子群划分保持初始不变
    class FixedConstructer final : public SubpopulationConstructer
    {
    public:
        FixedConstructer() : SubpopulationConstructer() {}
        ~FixedConstructer() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override {}
    };

    // 距离重聚类:每群抢占"离本群当前最优最近"的 Top-k 个体(k=子群大小),swap 迁入
    class DistanceConstructer final : public SubpopulationConstructer
    {
    public:
        DistanceConstructer() : SubpopulationConstructer() {}
        ~DistanceConstructer() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            int buffer_size = 0;
            for (int i = 0; i < swarm_number; i++)
                if (subpopulations[i]->getSize() > buffer_size)
                    buffer_size = subpopulations[i]->getSize();

            double* distance = new double[buffer_size + 1];
            int*    belong   = new int[buffer_size + 1];
            int*    index    = new int[buffer_size + 1];

            for (int i = 0; i < swarm_number; i++)
            {
                // 本群及其后各群的当前最优个体
                Individual* best = subpopulations[i]->getBestIndividualInSwarm();
                for (int j = i + 1; j < swarm_number; j++)
                {
                    Individual* cand = subpopulations[j]->getBestIndividualInSwarm();
                    if (*cand < *best) best = cand;
                }

                int subswarm_size = subpopulations[i]->getSize();
                int dim = best->getSolutionSize();

                // 方案 A 哨兵初始化:distance=MAX,belong/index 缺省指向本群自身第 j 个
                for (int j = 0; j < subswarm_size; j++) { distance[j] = ECFLOW_MAX; belong[j] = i; index[j] = j; }

                // Top-k:离 best 最近的 subswarm_size 个 (subpop,slot)
                for (int j = i; j < swarm_number; j++)
                {
                    for (int k = 0; k < subpopulations[j]->getSize(); k++)
                    {
                        double d = eu_distance(best->solution.result, (*subpopulations[j])[k].solution.result, dim);
                        if (d < distance[subswarm_size - 1])
                        {
                            int pos;
                            for (pos = subswarm_size - 1; pos > 0; pos--)
                            {
                                if (distance[pos - 1] > d)
                                {
                                    distance[pos] = distance[pos - 1];
                                    belong[pos]   = belong[pos - 1];
                                    index[pos]    = index[pos - 1];
                                }
                                else break;
                            }
                            distance[pos] = d; belong[pos] = j; index[pos] = k;
                        }
                    }
                }

                // 迁移:选中的换进本群(哨兵未覆盖 → 自我 swap = no-op)
                for (int j = 0; j < subswarm_size; j++)
                    (*subpopulations[i])[j].swap((*subpopulations[belong[j]])[index[j]]);
            }

            delete[] distance; delete[] belong; delete[] index;
        }
    };

    // 适应度重聚类:每群抢占全局适应度最好的 Top-k 个体(k=子群大小),swap 迁入
    class FitnessConstructer final : public SubpopulationConstructer
    {
    public:
        FitnessConstructer() : SubpopulationConstructer() {}
        ~FitnessConstructer() {}

        void build(Subpopulation** subpopulations, int& swarm_number) override
        {
            int buffer_size = 0;
            for (int i = 0; i < swarm_number; i++)
                if (subpopulations[i]->getSize() > buffer_size)
                    buffer_size = subpopulations[i]->getSize();

            Individual** best_list = new Individual * [buffer_size + 1];

            for (int i = 0; i < swarm_number; i++)
            {
                int subswarm_size = subpopulations[i]->getSize();

                // 先把本群前 subswarm_size 个按适应度插入排序(修 F-oob:标准插入,消 best_list[-1])
                for (int k = 0; k < subswarm_size; k++)
                {
                    Individual* cand = &(*subpopulations[i])[k];
                    int pos = k;
                    while (pos > 0 && *cand < *best_list[pos - 1]) { best_list[pos] = best_list[pos - 1]; pos--; }
                    best_list[pos] = cand;
                }

                // 再考虑 j>i 各群:优于当前第 subswarm_size 名则插入
                for (int j = i + 1; j < swarm_number; j++)
                {
                    for (int k = 0; k < subpopulations[j]->getSize(); k++)
                    {
                        Individual* cand = &(*subpopulations[j])[k];
                        if (*cand < *best_list[subswarm_size - 1])
                        {
                            int pos = subswarm_size - 1;
                            while (pos > 0 && *cand < *best_list[pos - 1]) { best_list[pos] = best_list[pos - 1]; pos--; }
                            best_list[pos] = cand;
                        }
                    }
                }

                // 迁移:Top-k 换进本群
                for (int j = 0; j < subswarm_size; j++)
                    (*subpopulations[i])[j].swap(*best_list[j]);
            }

            delete[] best_list;
        }
    };

    inline Registry<SubpopulationConstructer>::Entry fixedConstructerEntry()
    {
        return { "Fixed", ModuleType::T_subswarbuilder, ParameterTemplate{}, sizeof(FixedConstructer),
            [](const double*) -> SubpopulationConstructer* { return new FixedConstructer(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationConstructer>::Entry distanceConstructerEntry()
    {
        return { "Distance", ModuleType::T_subswarbuilder, ParameterTemplate{}, sizeof(DistanceConstructer),
            [](const double*) -> SubpopulationConstructer* { return new DistanceConstructer(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SubpopulationConstructer>::Entry fitnessConstructerEntry()
    {
        return { "Fitness", ModuleType::T_subswarbuilder, ParameterTemplate{}, sizeof(FitnessConstructer),
            [](const double*) -> SubpopulationConstructer* { return new FitnessConstructer(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(scon_fixed,    SubpopulationConstructer, fixedConstructerEntry());
    ECFLOW_REGISTER(scon_distance, SubpopulationConstructer, distanceConstructerEntry());
    ECFLOW_REGISTER(scon_fitness,  SubpopulationConstructer, fitnessConstructerEntry());
}
