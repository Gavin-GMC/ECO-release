//------------------------Description------------------------
// 蝗虫群(Grasshopper Optimization)学习策略 GrasshopperSwarm:每蝗虫受**所有其他蝗虫的两两社会力**(引/斥)之和
//   + 目标 T(=最优)吸引;舒适区系数 c 随进度收缩。O(n²) 为算法本性(每维遍历全群)。

//-------------------------Reference-------------------------
// S. Saremi, S. Mirjalili, A. Lewis, "Grasshopper Optimisation Algorithm: Theory and application,"
// Advances in Engineering Software, vol. 105, pp. 30-47, 2017, doi: 10.1016/j.advengsoft.2017.01.004.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "evoflow-rand.h"
#include "evoflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "terminator.h"
#include "registry.h"

namespace EvoFlow
{
    class GrasshopperSwarm : public LearningStrategy
    {
    private:
        double _f, _l;              // 社会力:引力强度(0.5)/引力尺度(1.5)
        double _c_max, _c_min;      // 舒适区系数界(1 / 4e-5;**罕调**)
        double _c;                  // 当代舒适区系数
        IndividualArray* _pop;      // 种群指针(preparation_s 存,遍历全群)

    public:
        GrasshopperSwarm(double f = 0.5, double l = 1.5, double c_max = 1.0, double c_min = 0.00004)
            : _f(f), _l(l), _c_max(c_max), _c_min(c_min), _c(c_max), _pop(nullptr) {}
        ~GrasshopperSwarm() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 目标 T(最优)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _c = _c_max; _pop = nullptr; }

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            double p = terminator ? terminator->getProgress() : 0.0;
            _c = _c_max - p * (_c_max - _c_min);
            _pop = &population;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object == nullptr || learning_object[0] == nullptr || _pop == nullptr) return x;  // 无目标/种群 → 不动
            double T = (*learning_object[0])[decision_d];
            if (is_empty(T)) return x;
            if (is_empty(x)) return T;
            double ub = problem_handle->getVariableUpbound(decision_d);
            double lb = problem_handle->getVariableLowbound(decision_d);
            const double eps = 1e-9;
            double half = (ub - lb) / 2.0;

            double sum = 0.0;
            int N = _pop->getSize();
            for (int j = 0; j < N; j++)
            {
                if (&(*_pop)[j] == individual) continue;                     // 跳过自身
                double xj = (*_pop)[j][decision_d];
                if (is_empty(xj)) continue;
                double diff = xj - x;
                double r = std::fabs(diff);
                double s = _f * std::exp(-r / _l) - std::exp(-r);            // 社会力
                sum += _c * half * s * diff / (r + eps);                     // 内 c
            }
            return _c * sum + T;                                             // 外 c + 目标
        }
    };

    inline Registry<LearningStrategy>::Entry grasshopperSwarmEntry()
    {
        return { "GrasshopperSwarm", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"f",     ParamKind::Real, 0.0, 2.0, false, 0.3, 0.7},
                                 {"l",     ParamKind::Real, 0.1, 5.0, false, 1.0, 2.0},
                                 {"c_max", ParamKind::Real, 0.0, 2.0, false, 0.5, 1.5},   // 罕调
                                 {"c_min", ParamKind::Real, 0.0, 1.0, false, 0.0, 0.001} } }, sizeof(GrasshopperSwarm),  // 罕调
            [](const double* p) -> LearningStrategy* { return p ? new GrasshopperSwarm(p[0], p[1], p[2], p[3]) : new GrasshopperSwarm(); },
            [](AssertList& L, const double* p) { GrasshopperSwarm::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { GrasshopperSwarm::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_goa, LearningStrategy, grasshopperSwarmEntry());
}
