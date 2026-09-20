//------------------------Description------------------------
// 白鲸优化(Beluga Whale Optimization)学习策略 BelugaWhalePhases:平衡因子 Bf 分相——游动(探索,维成对 sin/cos +
//   跨维打乱)、捕食(开发,Levy 向最优+随机)、鲸落(概率 Wf,随机+边界步)。向导来自 LeaderAndRandom(最优+随机)。

//-------------------------Reference-------------------------
// C. Zhong, G. Li, Z. Meng, "Beluga whale optimization: A novel nature-inspired metaheuristic
// algorithm," Knowledge-Based Systems, vol. 251, 109215, 2022, doi: 10.1016/j.knosys.2022.109215.

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
#include "strategy-levy.h"
#include "registry.h"

namespace EvoFlow
{
    class BelugaWhalePhases : public LearningStrategy
    {
    private:
        double _wf_max, _wf_min;   // 鲸落概率界(标准 0.1 / 0.05)
        double _b0_max;            // 平衡基 B0 的上界(**罕调标准 1**;B0∈[0,b0_max])
        double _c1_fac, _c2_fac;   // C1/C2 系数(**罕调标准各 2**)
        double _progress;
        int _popsize;

    public:
        BelugaWhalePhases(double wf_max = 0.1, double wf_min = 0.05, double b0_max = 1.0,
                          double c1_fac = 2.0, double c2_fac = 2.0)
            : _wf_max(wf_max), _wf_min(wf_min), _b0_max(b0_max), _c1_fac(c1_fac), _c2_fac(c2_fac),
              _progress(0.0), _popsize(1) {}
        ~BelugaWhalePhases() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需 2:最优+随机
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; _popsize = 1; }

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
            _popsize = population.getSize();
            if (_popsize < 1) _popsize = 1;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无最优 → 不动
            double Xbest = (*learning_object[0])[decision_d];
            double Xr = learning_object[1] ? (*learning_object[1])[decision_d] : Xbest;
            const double PI = 3.14159265358979323846;

            double Wf = _wf_max - (_wf_max - _wf_min) * _progress;
            if (rand01() < Wf)                                               // 鲸落(逐维)
            {
                double UB = problem_handle->getVariableUpbound(decision_d);
                double LB = problem_handle->getVariableLowbound(decision_d);
                double C2 = _c2_fac * Wf * _popsize;
                double Xstep = (UB - LB) * std::exp(-C2 * _progress);
                return rand01() * x - rand01() * Xr + rand01() * Xstep;
            }

            double B0 = rand01() * _b0_max;
            double Bf = B0 * (1.0 - _progress / 2.0);
            if (Bf > 0.5)                                                     // 游动(探索,跨维)
            {
                int n = problem_handle->getProblemSize(); if (n < 1) n = 1;
                int pj = EvoFlow::get_int(0, n - 1);
                int p1 = EvoFlow::get_int(0, n - 1);
                double xi_pj = (*individual)[pj];
                double xr_p1 = learning_object[1] ? (*learning_object[1])[p1] : Xbest;
                double factor = (xr_p1 - xi_pj) * (1.0 + rand01());
                double angle = 2.0 * PI * rand01();
                return (decision_d % 2 == 0) ? (xi_pj + factor * std::sin(angle))
                                             : (xi_pj + factor * std::cos(angle));
            }
            else                                                             // 捕食(开发,Levy)
            {
                double r3 = rand01(), r4 = rand01();
                double C1 = _c1_fac * r4 * (1.0 - _progress);
                return r3 * Xbest - r4 * x + C1 * levyFlight() * (Xr - x);
            }
        }
    };

    inline Registry<LearningStrategy>::Entry belugaWhalePhasesEntry()
    {
        return { "BelugaWhalePhases", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"wf_max", ParamKind::Real, 0.0, 1.0, false, 0.05, 0.2},
                                 {"wf_min", ParamKind::Real, 0.0, 1.0, false, 0.02, 0.1},
                                 {"b0_max", ParamKind::Real, 0.0, 5.0, false, 0.5, 1.5},   // 罕调:B0 上界
                                 {"c1_fac", ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0},   // 罕调:C1 系数
                                 {"c2_fac", ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0} } }, sizeof(BelugaWhalePhases),  // 罕调:C2 系数
            [](const double* p) -> LearningStrategy* { return p ? new BelugaWhalePhases(p[0], p[1], p[2], p[3], p[4]) : new BelugaWhalePhases(); },
            [](AssertList& L, const double* p) { BelugaWhalePhases::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { BelugaWhalePhases::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_bwo, LearningStrategy, belugaWhalePhasesEntry());
}
