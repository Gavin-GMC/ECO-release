//------------------------Description------------------------
// 灰狼优化(Grey Wolf Optimizer, GWO)学习策略 GreyWolfEncircling:向 3 个头狼(α/β/δ,当代种群前 3 名,
//   由 TopRanked 拓扑提供)包围收敛,系数 a 随运行进度在 [a_min,a_max] 间线性衰减(探索→开发)。
//-------------------------Reference-------------------------
// S. Mirjalili, S. M. Mirjalili, A. Lewis, "Grey Wolf Optimizer," Advances in Engineering Software,
// vol. 69, pp. 46-61, 2014, doi: 10.1016/j.advengsoft.2013.12.007.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include "ecflow-rand.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "terminator.h"
#include "registry.h"

namespace ECFlow
{
    class GreyWolfEncircling : public LearningStrategy
    {
    private:
        double _a_max, _a_min;   // 系数 a 的衰减边界(可调参数,标准 GWO 默认 2/0)
        double _a;               // 当代系数(preparation_s 算好)

    public:
        GreyWolfEncircling(double a_max = 2.0, double a_min = 0.0)
            : _a_max(a_max), _a_min(a_min), _a(a_max) {}
        ~GreyWolfEncircling() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // 需要 3 个头狼(α/β/δ)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _a = _a_max; }   // 每轮 exe 复位(同有状态策略惯例)

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _a = _a_max - (_a_max - _a_min) * progress;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            double x = (*individual)[decision_d];
            double sum = 0.0;
            int count = 0;
            for (int i = 0; i < 3; i++)
            {
                if (learning_object[i] == nullptr) continue;   // TopRanked 截断(种群 < 3)时跳过
                double L = (*learning_object[i])[decision_d];
                double r1 = rand01(), r2 = rand01();
                double A = 2.0 * _a * r1 - _a;
                double C = 2.0 * r2;
                double D = std::fabs(C * L - x);
                sum += L - A * D;
                count++;
            }
            return (count > 0) ? (sum / count) : x;   // 理论上 count 恒>=1(preAssert 要求 objects>=3)
        }
    };

    inline Registry<LearningStrategy>::Entry greyWolfEncirclingEntry()
    {
        return { "GreyWolfEncircling", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"a_max", ParamKind::Real, 0.0, 10.0, false, 1.0, 3.0},
                                 {"a_min", ParamKind::Real, 0.0, 10.0, false, 0.0, 0.5} } }, sizeof(GreyWolfEncircling),
            [](const double* p) -> LearningStrategy* { return p ? new GreyWolfEncircling(p[0], p[1]) : new GreyWolfEncircling(); },
            [](AssertList& L, const double* p) { GreyWolfEncircling::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { GreyWolfEncircling::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_greywolf, LearningStrategy, greyWolfEncirclingEntry());
}
