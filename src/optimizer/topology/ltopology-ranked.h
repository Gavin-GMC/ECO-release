//------------------------Description------------------------
// 排名学习拓扑 TopRanked:每代按 comparer(operator<,方向感知)对当代种群排序,取前 K 名
//   作为**全体个体共享**的学习对象(所有起点指向同一组 K 个终点)。K 任意,通用组件。
//-------------------------Reference-------------------------
// 无特定文献:通用"前 K 名"排名选择,供 GWO(K=3,α/β/δ)等多向导类策略复用。
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
#include "registry.h"

namespace ECFlow
{
    class TopRanked : public LearningTopology
    {
    private:
        int _k;
        std::vector<int> _rank_ids;   // 复用缓冲:当前最优列表(种群下标)

    public:
        explicit TopRanked(int k = 3) : LearningTopology(), _k(k) {}
        ~TopRanked() {}

        static void preAssert(AssertList& list, double* paras) {}   // 无个体类型要求

        static void postAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::postAssert));
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert)); // 起点=当前种群,规模=100%
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) override
        {
            IndividualArray* cswarm = subswarm[0];
            int graph_size = cswarm->getSize();
            int k = (_k < graph_size) ? _k : graph_size;   // K 超种群规模时截断

            // 维护大小 k 的当前最优下标列表(升序:_rank_ids[0] 最优)
            _rank_ids.clear();
            for (int i = 0; i < graph_size; i++)
            {
                if ((int)_rank_ids.size() < k)
                {
                    _rank_ids.push_back(i);
                    std::sort(_rank_ids.begin(), _rank_ids.end(),
                              [cswarm](int a, int b) { return (*cswarm)[a] < (*cswarm)[b]; });
                }
                else if ((*cswarm)[i] < (*cswarm)[_rank_ids.back()])
                {
                    _rank_ids.back() = i;
                    std::sort(_rank_ids.begin(), _rank_ids.end(),
                              [cswarm](int a, int b) { return (*cswarm)[a] < (*cswarm)[b]; });
                }
            }

            LearningGraph* back = new LearningGraph(graph_size, _k);   // 声明产出 objects=_k(原参数值,不因截断而变)
            for (int i = 0; i < graph_size; i++)
            {
                back->addStart(&(*cswarm)[i]);
                for (int j = 0; j < (int)_rank_ids.size(); j++)
                    back->addEnd(&(*cswarm)[_rank_ids[j]].solution);
                for (int j = (int)_rank_ids.size(); j < _k; j++)   // k 被截断时补空(生成器按 nullptr 视为不学习)
                    back->addEnd(nullptr);
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry topRankedEntry()
    {
        return { "TopRanked", ModuleType::T_learntopology,
            ParameterTemplate{ { {"leader_count", ParamKind::Int, 1, 0x3f3f3f3f, false, 1, 5} } }, sizeof(TopRanked),
            [](const double* p) -> LearningTopology* { return new TopRanked(p ? int(p[0]) : 3); },
            [](AssertList& L, const double* p) { TopRanked::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { TopRanked::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_topranked, LearningTopology, topRankedEntry());

    // 两端拓扑:每代取当代种群的**最优**与**最差**(按 comparer 的 operator<,方向感知)作为全体共享的学习对象——
    //   end[0]=best、end[1]=worst;起点=每个体自身。objects=2。与 TopRanked(只给"前 K 名")互补:此处需要"最差"这一端。
    //   通用组件,不含任何算法语义(behavior-name tag "BestAndWorst");供"向最优靠拢 + 远离最差"类策略复用。
    class BestAndWorst : public LearningTopology
    {
    public:
        BestAndWorst() : LearningTopology() {}
        ~BestAndWorst() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::postAssert));       // 供 best + worst
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));  // 起点=当前种群,规模 100%
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** /*best_holder*/,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            LearningGraph* back = new LearningGraph(n, 2);
            if (n <= 0) return back;

            int bi = 0, wi = 0;                       // 最优/最差下标(经 operator<,方向感知)
            for (int i = 1; i < n; i++)
            {
                if ((*cs)[i] < (*cs)[bi]) bi = i;     // 更优 → 新 best
                if ((*cs)[wi] < (*cs)[i]) wi = i;     // 当前 worst 优于 i → i 更差
            }
            for (int i = 0; i < n; i++)
            {
                back->addStart(&(*cs)[i]);
                back->addEnd(&(*cs)[bi].solution);    // end[0] = best
                back->addEnd(&(*cs)[wi].solution);    // end[1] = worst
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry bestAndWorstTopologyEntry()
    {
        return { "BestAndWorst", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(BestAndWorst),
            [](const double*) -> LearningTopology* { return new BestAndWorst(); },
            [](AssertList& L, const double* p) { BestAndWorst::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { BestAndWorst::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_bestandworst, LearningTopology, bestAndWorstTopologyEntry());

    // 每个体重复 k 次作起点(k 次采样机会)+ **当代最优**作学习对象(引导)。图规模 = (n−1)·k。
    //   当代最优自身不作起点(它与引导无差异,relink 无意义、白耗 FES)。多对一(同源 k 个)→ 配 kinshipGreedy 取每源最优。
    //   通用组件:凡"每个体向最优做 k 次采样式尝试"的算子(路径重连等)皆可复用。
    class RepeatWithBest : public LearningTopology
    {
    private:
        int _k;   // 每个体的采样次数
    public:
        explicit RepeatWithBest(int repeat_times = 5) : LearningTopology(), _k(repeat_times) {}
        ~RepeatWithBest() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));   // 供引导解
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** /*best_holder*/,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            if (n <= 1) return new LearningGraph(0, 1);

            int bi = 0;                                        // 当代最优(经 operator<,方向感知)
            for (int i = 1; i < n; i++) if ((*cs)[i] < (*cs)[bi]) bi = i;

            int k = (_k < 1) ? 1 : _k;
            LearningGraph* back = new LearningGraph((n - 1) * k, 1);
            for (int i = 0; i < n; i++)
            {
                if (i == bi) continue;                         // 最优自身跳过(与引导无差异)
                for (int j = 0; j < k; j++)
                {
                    back->addStart(&(*cs)[i]);                 // 同一起点重复 k 次 → 多对一,交血缘定向归属
                    back->addEnd(&(*cs)[bi].solution);         // 引导 = 当代最优
                }
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry repeatWithBestTopologyEntry()
    {
        return { "RepeatWithBest", ModuleType::T_learntopology,
            ParameterTemplate{ { {"repeat_times", ParamKind::Int, 1, 0x3f3f3f3f, false, 3, 8} } }, sizeof(RepeatWithBest),
            [](const double* p) -> LearningTopology* { return new RepeatWithBest(p ? (int)p[0] : 5); },
            [](AssertList& L, const double* p) { RepeatWithBest::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RepeatWithBest::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_repeatwithbest, LearningTopology, repeatWithBestTopologyEntry());
}
