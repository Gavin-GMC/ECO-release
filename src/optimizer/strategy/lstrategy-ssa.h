//------------------------Description------------------------
// 海鞘链(Salp Swarm)学习策略 SalpChainUpdate:领导(链首)跟食物源 F 移动、其余跟随者与链上前一只平均。
//   向导来自 SalpChain 拓扑(end[0]=食物 F、end[1]=前一只 salp,领导为空)。

//-------------------------Reference-------------------------
// S. Mirjalili, A. H. Gandomi, S. Z. Mirjalili, S. Saremi, H. Faris, S. M. Mirjalili, "Salp Swarm
// Algorithm: A bio-inspired optimizer for engineering design problems," Advances in Engineering
// Software, vol. 114, pp. 163-191, 2017, doi: 10.1016/j.advengsoft.2017.07.002.

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
#include "registry.h"

namespace EvoFlow
{
    class SalpChainUpdate : public LearningStrategy
    {
    private:
        double _c1_base, _c1_rate;   // c1 = c1_base·e^(−(c1_rate·p)²)(标准 2 / 4;c1_rate **罕调**)
        double _c1;

    public:
        SalpChainUpdate(double c1_base = 2.0, double c1_rate = 4.0) : _c1_base(c1_base), _c1_rate(c1_rate), _c1(c1_base) {}
        ~SalpChainUpdate() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 食物 + 前一只(领导端[1]空)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _c1 = _c1_base; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double p = terminator ? terminator->getProgress() : 0.0;
            _c1 = _c1_base * std::exp(-std::pow(_c1_rate * p, 2.0));
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无食物 → 不动

            if (learning_object[1] == nullptr)                              // 领导:跟食物
            {
                double F = (*learning_object[0])[decision_d];
                double ub = problem_handle->getVariableUpbound(decision_d);
                double lb = problem_handle->getVariableLowbound(decision_d);
                double term = (ub - lb) * rand01() + lb;                     // (ub−lb)·c2 + lb
                return (rand01() >= 0.5) ? (F + _c1 * term) : (F - _c1 * term);
            }
            // 跟随:与链上前一只(父代位置,并行版标注见头注)平均
            double ahead = (*learning_object[1])[decision_d];
            return (x + ahead) / 2.0;
        }
    };

    inline Registry<LearningStrategy>::Entry salpChainUpdateEntry()
    {
        return { "SalpChainUpdate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"c1_base", ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0},
                                 {"c1_rate", ParamKind::Real, 0.0, 10.0, false, 2.0, 6.0} } }, sizeof(SalpChainUpdate),  // c1_rate 罕调
            [](const double* p) -> LearningStrategy* { return p ? new SalpChainUpdate(p[0], p[1]) : new SalpChainUpdate(); },
            [](AssertList& L, const double* p) { SalpChainUpdate::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SalpChainUpdate::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_ssa, LearningStrategy, salpChainUpdateEntry());
}
