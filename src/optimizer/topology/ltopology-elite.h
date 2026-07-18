//------------------------Description------------------------
// 精英学习拓扑 EliteTopology:从最优档案取 elite_number 个精英,每个重复 repeat_times 次作为学习起点,
//   各自向自身(精英解)学习——用于在精英附近反复生成。
//-------------------------Reference-------------------------
// 无特定文献:精英学习(通用 elitism 思想——从最优档案取精英作为学习对象,在其附近反复生成)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <algorithm>
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    class EliteTopology : public LearningTopology
    {
    private:
        int _elite_number;
        int _repeat_times;

        Individual* population_buffer;

    public:
        EliteTopology(int elite_number, int repeat_times) : LearningTopology()
        {
            _elite_number = elite_number;
            _repeat_times = repeat_times;

            population_buffer = new Individual[_elite_number];
        }

        ~EliteTopology()
        {
            for (int i = 0; i < _elite_number; i++)
                population_buffer[i].shallowClear();

            delete[] population_buffer;
        }

        static void preAssert(AssertList& list, double* paras) {}
        // 非比例型:输出图规模 = elite_number×repeat_times(绝对值,与当前种群无关)→ 不声明 graphScale。
        static void postAssert(AssertList& list, double* paras) {}

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            int graph_size = _elite_number * _repeat_times;
            LearningGraph* back = new LearningGraph(graph_size, 1);

            for (int i = 0; i < _elite_number; i++)
            {
                population_buffer[i].solution.shallowCopy(*best_holder[0]->getElite());
                for (int j = 0; j < _repeat_times; j++)
                {
                    back->addStart(&population_buffer[i]);
                    back->addEnd(&population_buffer[i].solution);
                }
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry eliteTopologyEntry()
    {
        return { "Elite", ModuleType::T_learntopology,
            ParameterTemplate{ { {"elite_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 10},
                                 {"repeat_times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 5} } }, sizeof(EliteTopology),
            [](const double* p) -> LearningTopology* { return new EliteTopology(p ? int(p[0]) : 1, p ? int(p[1]) : 1); },
            [](AssertList& L, const double* p) { EliteTopology::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { EliteTopology::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_elite, LearningTopology, eliteTopologyEntry());

    // 种群 top-e 精英作起点(局搜强化):取当代种群前 e 名(fitness),每精英作**起点**重复 r 次——
    //   起点 = **种群槽** `&pop[精英槽]`(非内部 buffer),故 RunGenerate 通用盖 `id=槽` → 邻居出生派生 `parent_id=精英槽`,
    //   供 `KinshipSelector` 归属回各自精英、择优替换(E6)。端点 = 随机同伴 k≠精英(差分算子用/变异算子忽略,两者皆可)。
    //   与 `Elite`(档案精英,内部 buffer,配 Rank 全局归并)区分:此为**种群精英**、起点=真种群槽、血缘可归属。
    class TopIndividual : public LearningTopology
    {
    private:
        int _elite_number;
        int _repeat_times;
        std::vector<int> _rank_ids;   // top-e 下标(升序:[0] 最优)
    public:
        TopIndividual(int elite_number, int repeat_times) : LearningTopology(), _elite_number(elite_number), _repeat_times(repeat_times) {}
        ~TopIndividual() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));   // 供 1 同伴
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** /*best_holder*/,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            int e = (_elite_number < n) ? _elite_number : n;   // 精英数截断到种群规模

            // 维护 top-e 下标(升序,[0] 最优;同 TopRanked)
            _rank_ids.clear();
            for (int i = 0; i < n; i++)
            {
                if ((int)_rank_ids.size() < e)
                {
                    _rank_ids.push_back(i);
                    std::sort(_rank_ids.begin(), _rank_ids.end(), [cs](int a, int b) { return (*cs)[a] < (*cs)[b]; });
                }
                else if ((*cs)[i] < (*cs)[_rank_ids.back()])
                {
                    _rank_ids.back() = i;
                    std::sort(_rank_ids.begin(), _rank_ids.end(), [cs](int a, int b) { return (*cs)[a] < (*cs)[b]; });
                }
            }

            LearningGraph* g = new LearningGraph(e * _repeat_times, 1);
            for (int t = 0; t < e; t++)
            {
                int elite = _rank_ids[t];
                for (int j = 0; j < _repeat_times; j++)
                {
                    g->addStart(&(*cs)[elite]);                       // 起点=精英种群槽(kinship 盖 id=elite 槽)
                    int k = get_int(0, n - 1);
                    while (k == elite && n > 1) k = get_int(0, n - 1);  // 随机同伴 k≠精英
                    g->addEnd(&(*cs)[k].solution);
                }
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry topIndividualTopologyEntry()
    {
        return { "TopIndividual", ModuleType::T_learntopology,
            ParameterTemplate{ { {"elite_number", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 10},
                                 {"repeat_times", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 5} } }, sizeof(TopIndividual),
            [](const double* p) -> LearningTopology* { return new TopIndividual(p ? int(p[0]) : 1, p ? int(p[1]) : 1); },
            [](AssertList& L, const double* p) { TopIndividual::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { TopIndividual::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_topindividual, LearningTopology, topIndividualTopologyEntry());
}
