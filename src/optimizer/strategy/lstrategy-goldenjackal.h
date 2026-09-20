//------------------------Description------------------------
// 金豺钳形(Golden Jackal)学习策略 GoldenJackalPincer:雌雄双豺(X_M=最优、X_F=次优,TopRanked(2) 提供)协同
//   钳形逼近猎物(个体自身);逃逸能量 E=E1·E0 随进度衰减,|E|≥1 探索 / |E|<1 开发;含 Levy 飞行随机 rl。

//-------------------------Reference-------------------------
// N. Chopra, M. M. Ansari, "Golden jackal optimization: A novel nature-inspired optimizer for
// engineering applications," Expert Systems with Applications, vol. 198, 116924, 2022,
// doi: 10.1016/j.eswa.2022.116924.

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
    class GoldenJackalPincer : public LearningStrategy
    {
    private:
        double _c1;     // 能量初值系数(标准 1.5)
        double _beta;   // Levy 飞行形状参数(标准 1.5)
        double _e1;     // 当代能量因子(preparation_s 算好)

    public:
        GoldenJackalPincer(double c1 = 1.5, double beta = 1.5) : _c1(c1), _beta(beta), _e1(c1) {}
        ~GoldenJackalPincer() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需 2:雌雄双豺
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _e1 = _c1; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _e1 = _c1 * (1.0 - progress);
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            double prey = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return prey;                        // 无最优 → 不动
            double XM = (*learning_object[0])[decision_d];                         // 雄豺=最优
            double XF = learning_object[1] ? (*learning_object[1])[decision_d] : XM; // 雌豺=次优(缺→XM)
            double E0 = 2.0 * rand01() - 1.0;
            double E  = _e1 * E0;
            double rl = 0.05 * levyFlight(_beta);
            double Y1, Y2;
            if (std::fabs(E) >= 1.0)                                               // 探索
            {
                Y1 = XM - E * std::fabs(XM - rl * prey);
                Y2 = XF - E * std::fabs(XF - rl * prey);
            }
            else                                                                  // 开发
            {
                Y1 = XM - E * std::fabs(rl * XM - prey);
                Y2 = XF - E * std::fabs(rl * XF - prey);
            }
            return (Y1 + Y2) / 2.0;
        }
    };

    inline Registry<LearningStrategy>::Entry goldenJackalPincerEntry()
    {
        return { "GoldenJackalPincer", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"c1",   ParamKind::Real, 0.0, 5.0, false, 1.0, 2.0},
                                 {"beta", ParamKind::Real, 0.1, 2.0, false, 1.0, 1.8} } }, sizeof(GoldenJackalPincer),
            [](const double* p) -> LearningStrategy* { return p ? new GoldenJackalPincer(p[0], p[1]) : new GoldenJackalPincer(); },
            [](AssertList& L, const double* p) { GoldenJackalPincer::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { GoldenJackalPincer::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_goldenjackal, LearningStrategy, goldenJackalPincerEntry());
}
