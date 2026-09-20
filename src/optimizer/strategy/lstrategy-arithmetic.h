//------------------------Description------------------------
// 四则运算(Arithmetic)学习策略 ArithmeticOperatorUpdate:围绕当代最优(单向导,TopRanked(1))用 ÷×+− 四算子
//   做位置更新,由两个进度系数门控——MOA(随进度递增,选相位)与 MOP(随进度递减,控步长)。

//-------------------------Reference-------------------------
// L. Abualigah, A. Diabat, S. Mirjalili, M. Abd Elaziz, A. H. Gandomi, "The Arithmetic Optimization
// Algorithm," Computer Methods in Applied Mechanics and Engineering, vol. 376, 113609, 2021,
// doi: 10.1016/j.cma.2020.113609.

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
    class ArithmeticOperatorUpdate : public LearningStrategy
    {
    private:
        double _alpha;              // 开发精度指数(MOP 的 1/alpha 次幂,标准 5)
        double _moa_min, _moa_max;  // MOA 递增边界(标准 0.2 / 1.0)
        double _mu;                 // 边界缩放系数(标准 0.5)
        double _moa, _mop;          // 当代系数(preparation_s 算好)

    public:
        ArithmeticOperatorUpdate(double alpha = 5.0, double moa_min = 0.2, double moa_max = 1.0, double mu = 0.5)
            : _alpha(alpha), _moa_min(moa_min), _moa_max(moa_max), _mu(mu), _moa(moa_min), _mop(1.0) {}
        ~ArithmeticOperatorUpdate() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个目标点(当代最优)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _moa = _moa_min; _mop = 1.0; }   // 每轮 exe 复位(progress=0)

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _moa = _moa_min + progress * (_moa_max - _moa_min);
            _mop = 1.0 - std::pow(progress, 1.0 / _alpha);
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            if (learning_object[0] == nullptr) return (*individual)[decision_d];   // 无目标点(空种群)→ 不动
            double P  = (*learning_object[0])[decision_d];
            double UB = problem_handle->getVariableUpbound(decision_d);
            double LB = problem_handle->getVariableLowbound(decision_d);
            double term = (UB - LB) * _mu + LB;
            const double eps = 1e-9;
            double r1 = rand01(), r2 = rand01(), r3 = rand01();
            if (r1 > _moa)                                   // 探索:高散布(÷/×)
                return (r2 < 0.5) ? (P / (_mop + eps) * term) : (P * _mop * term);
            else                                             // 开发:高密度(-/+)
                return (r3 < 0.5) ? (P - _mop * term) : (P + _mop * term);
        }
    };

    inline Registry<LearningStrategy>::Entry arithmeticOperatorUpdateEntry()
    {
        return { "ArithmeticOperatorUpdate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha",   ParamKind::Real, 1.0, 20.0, false, 3.0, 10.0},
                                 {"moa_min", ParamKind::Real, 0.0,  1.0, false, 0.1,  0.3},
                                 {"moa_max", ParamKind::Real, 0.0,  1.0, false, 0.8,  1.0},
                                 {"mu",      ParamKind::Real, 0.0,  2.0, false, 0.3,  0.7} } }, sizeof(ArithmeticOperatorUpdate),
            [](const double* p) -> LearningStrategy* { return p ? new ArithmeticOperatorUpdate(p[0], p[1], p[2], p[3]) : new ArithmeticOperatorUpdate(); },
            [](AssertList& L, const double* p) { ArithmeticOperatorUpdate::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ArithmeticOperatorUpdate::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_arithmetic, LearningStrategy, arithmeticOperatorUpdateEntry());
}
