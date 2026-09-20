//------------------------Description------------------------
// 秩极点+三随机学习拓扑(两款,共享端构建):每起点给 6 个学习对象——
//   end[0/1/2]=全局 best/better(次优)/worst(方向感知)、end[3/4/5]=3 个随机个体(**已按优劣排序**,供加权均值直接用)。
//   RankExtremesTriRandom          起点=当代种群(供 INFO 更新段 WeightedMeanUpdate)。
//   ContinueRankExtremesTriRandom  起点=上段子代(续接,供 INFO 局搜段 WeightedMeanLocalSearch)。
//   为"用加权均值方向(WM)"类策略提供数据(6 对象含 fitness);通用可复用。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <utility>
#include "learning-topology.h"
#include "best-archive.h"
#include "evoflow-rand.h"
#include "registry.h"

namespace EvoFlow
{
    namespace rankextremes_detail
    {
        // 找 best/better/worst(方向感知 operator<)。
        inline void findExtremes(IndividualArray& cs, int N, Solution*& bs, Solution*& bt, Solution*& ws)
        {
            int ib = 0, iw = 0;
            for (int i = 1; i < N; i++)
            {
                if (cs[i] < cs[ib]) ib = i;           // 更优 → best
                if (cs[iw] < cs[i]) iw = i;           // 更劣 → worst
            }
            int ibt = -1;
            for (int i = 0; i < N; i++)               // better = 除 best 外最优
            {
                if (i == ib) continue;
                if (ibt == -1 || cs[i] < cs[ibt]) ibt = i;
            }
            if (ibt == -1) ibt = ib;
            bs = &cs[ib].solution; bt = &cs[ibt].solution; ws = &cs[iw].solution;
        }

        // 给一条 graph 追加 6 个 end:best/better/worst + 3 随机(排序,best 在前)。
        inline void appendEnds(LearningGraph* g, IndividualArray& cs, int N, Solution* bs, Solution* bt, Solution* ws)
        {
            g->addEnd(bs); g->addEnd(bt); g->addEnd(ws);
            int r[3] = { EvoFlow::get_int(0, N - 1), EvoFlow::get_int(0, N - 1), EvoFlow::get_int(0, N - 1) };
            for (int a = 0; a < 3; a++)
                for (int b = a + 1; b < 3; b++)
                    if (cs[r[b]] < cs[r[a]]) std::swap(r[a], r[b]);   // 3 元排序(优→劣)
            g->addEnd(&cs[r[0]].solution); g->addEnd(&cs[r[1]].solution); g->addEnd(&cs[r[2]].solution);
        }
    }

    // ① 起点=当代种群
    class RankExtremesTriRandom : public LearningTopology
    {
    public:
        RankExtremesTriRandom() : LearningTopology() {}
        ~RankExtremesTriRandom() {}
        static void preAssert(AssertList&, double*) {}
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 6, MatchType::postAssert));      // best/better/worst + 3 随机
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }
        void ini() override {}
        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive**,
                                   const int, IndividualArray*, LearningGraph*) override
        {
            IndividualArray* cs = subswarm[0];
            int N = cs->getSize();
            LearningGraph* g = new LearningGraph(N, 6);
            if (N == 0) return g;
            Solution* bs; Solution* bt; Solution* ws;
            rankextremes_detail::findExtremes(*cs, N, bs, bt, ws);
            for (int i = 0; i < N; i++)
            {
                g->addStart(&(*cs)[i]);
                rankextremes_detail::appendEnds(g, *cs, N, bs, bt, ws);
            }
            return g;
        }
    };

    // ② 起点=上段子代(续接);端仍取当代种群
    class ContinueRankExtremesTriRandom : public LearningTopology
    {
    public:
        ContinueRankExtremesTriRandom() : LearningTopology() {}
        ~ContinueRankExtremesTriRandom() {}
        // 起点=当前 offspring(见 getTopology)，故显式要求 offspringAvailable；
        // Selector 清空 offspring 后不可续接，也不能作首个/唯一生成源。
        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_offspringgenerator, "offspringAvailable", 1, MatchType::notLess));
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 6, MatchType::postAssert));      // best/better/worst + 3 随机
            list.add(new Assert(ModuleType::T_learntopology, "offspringStart", 1, MatchType::postAssert)); // 起点=当前 offspring,支持 inplace 续接
        }
        void ini() override {}
        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive**,
                                   const int, IndividualArray* offspring, LearningGraph*) override
        {
            IndividualArray* cs = subswarm[0];
            int N = cs->getSize();
            int M = offspring ? offspring->getSize() : 0;
            LearningGraph* g = new LearningGraph(M, 6);
            if (N == 0 || M == 0) return g;
            Solution* bs; Solution* bt; Solution* ws;
            rankextremes_detail::findExtremes(*cs, N, bs, bt, ws);
            for (int i = 0; i < M; i++)
            {
                g->addStart(&(*offspring)[i]);                          // 起点=上段子代 u
                rankextremes_detail::appendEnds(g, *cs, N, bs, bt, ws);   // 端=种群极点+随机
            }
            return g;
        }
    };

    inline Registry<LearningTopology>::Entry rankExtremesTriRandomEntry()
    {
        return { "RankExtremesTriRandom", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(RankExtremesTriRandom),
            [](const double*) -> LearningTopology* { return new RankExtremesTriRandom(); },
            [](AssertList& L, const double* p) { RankExtremesTriRandom::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RankExtremesTriRandom::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningTopology>::Entry continueRankExtremesTriRandomEntry()
    {
        return { "ContinueRankExtremesTriRandom", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(ContinueRankExtremesTriRandom),
            [](const double*) -> LearningTopology* { return new ContinueRankExtremesTriRandom(); },
            [](AssertList& L, const double* p) { ContinueRankExtremesTriRandom::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ContinueRankExtremesTriRandom::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(ltopo_rankextremestrirandom, LearningTopology, rankExtremesTriRandomEntry());
    EVOFLOW_REGISTER(ltopo_continuerankextremestrirandom, LearningTopology, continueRankExtremesTriRandomEntry());
}
