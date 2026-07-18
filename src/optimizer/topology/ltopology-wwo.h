//------------------------Description------------------------
// 水波类两个激活拓扑(源自 Water Wave Optimization 的折射/碎波触发条件):
//   WaveRefractionActivation —— 波高 h 降到 0 的波才激活,学习对象 = **档案最优 x***;其余空指针透传。
//   NewBestActivation        —— **自持上一轮档案最优的拷贝**,仅当本轮出现"新最优"才激活(对该最优个体产 k 个孤立波)。
//-------------------------Reference-------------------------
// Zheng, Y.-J. "Water wave optimization: A new nature-inspired metaheuristic."
//   Computers & Operations Research, 55:1-11, 2015.
//   折射条件:h 降到 0;折射用 x*(至今最优)。碎波条件:传播**产生新最优**时对其碎波(非每代)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "learning-topology.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 折射激活:h≤0 的波 → 起点=自身、学习对象=档案最优 x*;其余 → 空指针透传
    class WaveRefractionActivation : public LearningTopology
    {
    public:
        WaveRefractionActivation() : LearningTopology() {}
        ~WaveRefractionActivation() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));       // 供 x*
            list.add(new Assert(ModuleType::T_learntopology, "graphScale", 100, MatchType::postAssert));
        }

        void ini() override {}

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            LearningGraph* back = new LearningGraph(n, 1);
            Solution* elite = (best_holder && best_holder[0]) ? best_holder[0]->getElite() : nullptr;

            for (int i = 0; i < n; i++)
            {
                back->addStart(&(*cs)[i]);
                bool refract = elite && (*cs)[i].hasFeature("wavestate")
                            && (*cs)[i].feature<ScalarFeature>("wavestate")->vals[0] <= 0.0;   // h 降到 0
                if (refract) back->addEnd(elite);        // 折射:学习对象 = 档案最优 x*
                else         back->addEnd(nullptr);      // 未激活:空指针契约 → copy 进子代、不评估
            }
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry waveRefractionActivationEntry()
    {
        return { "WaveRefractionActivation", ModuleType::T_learntopology, ParameterTemplate{}, sizeof(WaveRefractionActivation),
            [](const double*) -> LearningTopology* { return new WaveRefractionActivation(); },
            [](AssertList& L, const double* p) { WaveRefractionActivation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WaveRefractionActivation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_waverefractionactivation, LearningTopology, waveRefractionActivationEntry());

    // 新最优激活:自持上轮档案最优拷贝;仅当本轮 elite 更优(出了新最优)→ 对种群最优个体产 k~U[1,k_max] 个起点
    class NewBestActivation : public LearningTopology
    {
    private:
        int        _k_max;
        Individual _last_elite;   // 自持:上一轮档案最优的**拷贝**(用 Individual 承载,借种群个体的 comparer 以 operator< 比较)
        Individual _cur_elite;    // 当前 elite 的承载(同上)
        bool       _has_last;
        bool       _bound;        // 是否已借到 comparer/规模

    public:
        explicit NewBestActivation(int k_max = 12) : LearningTopology(), _k_max(k_max), _has_last(false), _bound(false) {}
        ~NewBestActivation() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::postAssert));
        }

        void ini() override { _has_last = false; }   // 每轮 exe 复位自持拷贝

        LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                   const int /*swarm_number*/, IndividualArray* /*offspring*/, LearningGraph* /*last_graph*/) override
        {
            IndividualArray* cs = subswarm[0];
            int n = cs->getSize();
            Solution* elite = (best_holder && best_holder[0]) ? best_holder[0]->getElite() : nullptr;
            if (!elite || n <= 0) return new LearningGraph(0, 1);

            if (!_bound)   // 借种群个体的 comparer + 规模(拓扑基类拿不到句柄,故经个体借)
            {
                _last_elite.setProblem((*cs)[0]);
                _cur_elite.setProblem((*cs)[0]);
                _bound = true;
            }

            _cur_elite.solution.copy(*elite);
            bool new_best = (!_has_last) || (_cur_elite < _last_elite);   // 首代视为新最优;否则比自持拷贝更优即"出了新最优"

            if (!new_best) return new LearningGraph(0, 1);   // 未出新最优 → 空图(零生成/零评估)

            // 找当代最优个体(它即刚成为新最优的那个波)
            int bi = 0;
            for (int i = 1; i < n; i++) if ((*cs)[i] < (*cs)[bi]) bi = i;

            int k = get_int(1, (_k_max < 1) ? 1 : _k_max);   // k ~ U[1, k_max] 个孤立波
            LearningGraph* back = new LearningGraph(k, 1);
            for (int j = 0; j < k; j++)
            {
                back->addStart(&(*cs)[bi]);                  // 多对一:k 个孤立波同源 → 由 kinshipGreedy 归属回它
                back->addEnd(&(*cs)[bi].solution);           // 端点=自身(非空,避开透传契约;碎波策略不读它)
            }

            _last_elite.solution.copy(*elite);               // 刷新自持拷贝
            _has_last = true;
            return back;
        }
    };

    inline Registry<LearningTopology>::Entry newBestActivationEntry()
    {
        return { "NewBestActivation", ModuleType::T_learntopology,
            ParameterTemplate{ { {"k_max", ParamKind::Int, 1, 0x3f3f3f3f, false, 4, 12} } }, sizeof(NewBestActivation),
            [](const double* p) -> LearningTopology* { return new NewBestActivation(p ? (int)p[0] : 12); },
            [](AssertList& L, const double* p) { NewBestActivation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { NewBestActivation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(ltopo_newbestactivation, LearningTopology, newBestActivationEntry());
}
