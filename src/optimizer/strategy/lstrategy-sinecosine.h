//------------------------Description------------------------
// 正弦余弦振荡(Sine-Cosine)学习策略 SineCosineOscillation:向当代最优(单向导,TopRanked(1) 提供)以
//   正弦/余弦振荡逐步逼近,振幅系数 r1 随运行进度线性衰减(探索→开发)。

//-------------------------Reference-------------------------
// S. Mirjalili, "SCA: A Sine Cosine Algorithm for solving optimization problems,"
// Knowledge-Based Systems, vol. 96, pp. 120-133, 2016, doi: 10.1016/j.knosys.2015.12.022.

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
    class SineCosineOscillation : public LearningStrategy
    {
    private:
        double _a;    // 振幅系数上界(可调参数,标准 SCA 默认 2)
        double _r1;   // 当代振幅(preparation_s 算好)

    public:
        explicit SineCosineOscillation(double a = 2.0) : _a(a), _r1(a) {}
        ~SineCosineOscillation() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个目标点(当代最优)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _r1 = _a; }   // 每轮 exe 复位(progress=0 → r1=a)

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _r1 = _a * (1.0 - progress);
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            if (learning_object[0] == nullptr) return (*individual)[decision_d];   // 无目标点(空种群)→ 不动
            double x = (*individual)[decision_d];
            double P = (*learning_object[0])[decision_d];
            double r2 = rand01() * 6.283185307179586;   // [0,2π)
            double r3 = rand01() * 2.0;                 // [0,2)
            double r4 = rand01();                       // sin/cos 分支
            double diff = std::fabs(r3 * P - x);
            return x + _r1 * ((r4 < 0.5) ? std::sin(r2) : std::cos(r2)) * diff;
        }
    };

    inline Registry<LearningStrategy>::Entry sineCosineOscillationEntry()
    {
        return { "SineCosineOscillation", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"a", ParamKind::Real, 0.0, 10.0, false, 1.0, 3.0} } }, sizeof(SineCosineOscillation),
            [](const double* p) -> LearningStrategy* { return p ? new SineCosineOscillation(p[0]) : new SineCosineOscillation(); },
            [](AssertList& L, const double* p) { SineCosineOscillation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SineCosineOscillation::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_sinecosine, LearningStrategy, sineCosineOscillationEntry());
}
