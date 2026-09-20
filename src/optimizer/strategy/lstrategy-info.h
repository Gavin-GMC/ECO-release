//------------------------Description------------------------
// INFO(weIghted meaN oF vectOrs)拆成两个可组合策略,共享 strategy-weightedmean.h 的 weightedMeanVectors 原语:
//   WeightedMeanUpdate       —— INFO 阶段1+2(更新规则 + 向量组合):以加权均值 MeanRule 驱动,产 z1/z2 合成 u。
//   WeightedMeanLocalSearch  —— INFO 阶段3(局部搜索,**通用可复用**):概率性绕最优/随机点用加权均值方向 + 高斯精修。
//   典型编排:段A RankExtremesTriRandom + WeightedMeanUpdate → 段B ContinueRankExtremesTriRandom + WeightedMeanLocalSearch(续接)。

//-------------------------Reference-------------------------
// M. Ahmadianfar, A. A. Heidari, A. H. Gandomi, X. Chu, H. Chen, "INFO: An efficient optimization
// algorithm based on weighted mean of vectors," Expert Systems with Applications, vol. 195, 116516, 2022,
// doi: 10.1016/j.eswa.2022.116516.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "evoflow-rand.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "terminator.h"
#include "strategy-weightedmean.h"
#include "registry.h"

namespace EvoFlow
{
    // 逐维算 MeanRule = r·WM1(3随机) + (1−r)·WM2(极点) + ε·randn(供两段共用)。
    inline double infoMeanRule(Solution** lo, int d)
    {
        double WM1 = (lo[3] && lo[4] && lo[5]) ? weightedMeanVectors(lo[3], lo[4], lo[5], d) : 0.0;
        Solution* b = lo[0];
        Solution* bt = lo[1] ? lo[1] : lo[0];
        Solution* ws = lo[2] ? lo[2] : lo[0];
        double WM2 = weightedMeanVectors(b, bt, ws, d);
        double r = rand01();
        return r * WM1 + (1.0 - r) * WM2 + 1e-9 * get_normal(0.0, 1.0);
    }

    // ===== INFO 阶段1+2:更新规则 + 向量组合 =====
    class WeightedMeanUpdate : public LearningStrategy
    {
    private:
        double _a_base, _a_rate;   // α = a_base·e^(−a_rate·p)(标准 2 / 4;a_rate **罕调**)
        double _alpha;

    public:
        WeightedMeanUpdate(double a_base = 2.0, double a_rate = 4.0) : _a_base(a_base), _a_rate(a_rate), _alpha(a_base) {}
        ~WeightedMeanUpdate() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 6, MatchType::notLessButNotice)); // best/better/worst+3随机
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));
        }

        void ini(ProblemHandle*) override { _alpha = _a_base; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double p = terminator ? terminator->getProgress() : 0.0;
            _alpha = _a_base * std::exp(-_a_rate * p);
        }

        double nextDecision(const int d, Individual* individual, Solution** lo, ProblemHandle*, Individual*) override
        {
            double x = (*individual)[d];
            if (lo[0] == nullptr) return x;
            double MeanRule = infoMeanRule(lo, d);
            double sigma = 2.0 * _alpha * rand01() - _alpha;

            double best   = (*lo[0])[d];
            double better = lo[1] ? (*lo[1])[d] : best;
            double a1 = lo[3] ? (*lo[3])[d] : best;
            double a2 = lo[4] ? (*lo[4])[d] : a1;
            double a3 = lo[5] ? (*lo[5])[d] : a2;
            double fb = lo[0]->fitness[0];
            double f1 = lo[3] ? lo[3]->fitness[0] : fb;
            double f2 = lo[4] ? lo[4]->fitness[0] : f1;
            double f3 = lo[5] ? lo[5]->fitness[0] : f2;

            double z1, z2;
            if (rand01() < 0.5)
            {
                z1 = x    + sigma * MeanRule + rand01() * (best - a1) / (fb - f1 + 1.0);
                z2 = best + sigma * MeanRule + rand01() * (a1 - a2)  / (f1 - f2 + 1.0);
            }
            else
            {
                z1 = a1     + sigma * MeanRule + rand01() * (a2 - a3) / (f2 - f3 + 1.0);
                z2 = better + sigma * MeanRule + rand01() * (a1 - a2) / (f1 - f2 + 1.0);
            }
            double mu = 0.05 * get_normal(0.0, 1.0);                            // 向量组合
            return (rand01() < 0.5) ? (z1 + mu * std::fabs(z1 - z2)) : (z2 + mu * std::fabs(z1 - z2));
        }
    };

    // ===== INFO 阶段3:加权均值局部搜索(通用可复用;概率施用,否则原样传出) =====
    class WeightedMeanLocalSearch : public LearningStrategy
    {
    private:
        double _prob;   // 施用局搜的概率(标准 0.5)

    public:
        explicit WeightedMeanLocalSearch(double prob = 0.5) : _prob(prob) {}
        ~WeightedMeanLocalSearch() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 6, MatchType::notLessButNotice));
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));
        }

        double nextDecision(const int d, Individual* individual, Solution** lo, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[d];                                       // = 上段输出 u
            if (lo[0] == nullptr || rand01() >= _prob) return x;               // 不施局搜 → 原样传出
            double MeanRule = infoMeanRule(lo, d);
            double best = (*lo[0])[d];
            double a1 = lo[3] ? (*lo[3])[d] : best;
            if (rand01() < 0.5)                                                // 绕最优精修
                return best + get_normal(0.0, 1.0) * (MeanRule + get_normal(0.0, 1.0) * (best - a1));
            double UB = problem_handle->getVariableUpbound(d);                 // 绕随机新点(跳出)
            double LB = problem_handle->getVariableLowbound(d);
            double xrnd = LB + rand01() * (UB - LB);
            return xrnd + get_normal(0.0, 1.0) * (MeanRule + get_normal(0.0, 1.0) * (rand01() * best - rand01() * xrnd));
        }
    };

    inline Registry<LearningStrategy>::Entry weightedMeanUpdateEntry()
    {
        return { "WeightedMeanUpdate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"a_base", ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0},
                                 {"a_rate", ParamKind::Real, 0.0, 10.0, false, 2.0, 6.0} } }, sizeof(WeightedMeanUpdate),  // a_rate 罕调
            [](const double* p) -> LearningStrategy* { return p ? new WeightedMeanUpdate(p[0], p[1]) : new WeightedMeanUpdate(); },
            [](AssertList& L, const double* p) { WeightedMeanUpdate::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WeightedMeanUpdate::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry weightedMeanLocalSearchEntry()
    {
        return { "WeightedMeanLocalSearch", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"prob", ParamKind::Real, 0.0, 1.0, false, 0.3, 0.7} } }, sizeof(WeightedMeanLocalSearch),
            [](const double* p) -> LearningStrategy* { return p ? new WeightedMeanLocalSearch(p[0]) : new WeightedMeanLocalSearch(); },
            [](AssertList& L, const double* p) { WeightedMeanLocalSearch::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WeightedMeanLocalSearch::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_info_update,      LearningStrategy, weightedMeanUpdateEntry());
    EVOFLOW_REGISTER(lstrat_info_localsearch, LearningStrategy, weightedMeanLocalSearchEntry());
}
