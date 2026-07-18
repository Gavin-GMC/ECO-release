//------------------------Description------------------------
// "向最优靠拢 + 远离最差"更新策略 BestWorstGuided(源自 Jaya)。**无算法专属参数**(parameter-free):
//   每维取两个 [0,1] 随机数,一项拉向当代最优、一项推离当代最差。配 BestAndWorst 拓扑(end[0]=best、end[1]=worst)。
//-------------------------Reference-------------------------
// Rao, R.V. "Jaya: A simple and new optimization algorithm for solving constrained and unconstrained
//   optimization problems." International Journal of Industrial Engineering Computations, 7(1):19-34, 2016.
// 忠实原式(逐变量、逐代取随机):
//   X'_{j,k} = X_{j,k} + r1_j·(X_{j,best} − |X_{j,k}|) − r2_j·(X_{j,worst} − |X_{j,k}|)
//   其中 |·| 为绝对值(原文如此);r1_j, r2_j ~ U[0,1] 每变量每代独立取。
// 原文的"仅当更优才接受"(贪心)由**选择器**承担(selector.index(true)),非本算子职责——与框架两层分工一致。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // X' = X + r1·(best − |X|) − r2·(worst − |X|),r1/r2 逐维逐代 ~U[0,1]。无参数。
    class BestWorstGuided : public LearningStrategy
    {
    public:
        BestWorstGuided() {}
        ~BestWorstGuided() {}

        static void preAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice));   // 需 best + worst
        }
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));    // 逐维构造
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object,
                            ProblemHandle*, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr || learning_object[1] == nullptr) return x;   // 防御:无两端则保持

            double best  = (*learning_object[0])[decision_d];
            double worst = (*learning_object[1])[decision_d];
            double r1 = rand01(), r2 = rand01();                                            // 逐变量逐代独立
            double ax = std::fabs(x);                                                       // 原文用绝对值
            return x + r1 * (best - ax) - r2 * (worst - ax);
        }
    };

    inline Registry<LearningStrategy>::Entry bestWorstGuidedEntry()
    {
        return { "BestWorstGuided", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(BestWorstGuided),   // 无参数
            [](const double*) -> LearningStrategy* { return new BestWorstGuided(); },
            [](AssertList& L, const double* p) { BestWorstGuided::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { BestWorstGuided::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_bestworstguided, LearningStrategy, bestWorstGuidedEntry());
}
