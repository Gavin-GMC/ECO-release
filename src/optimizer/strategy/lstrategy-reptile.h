//------------------------Description------------------------
// 爬行动物包围(Reptile Search)学习策略 ReptileEncircling:模拟鳄鱼包围(探索)→ 狩猎(开发),按运行进度分 4 相位。
//   向导来自 LeaderRandomCenter 拓扑(end[0]=最优 P、end[1]=随机成员 x_r、end[2]=逐维种群均值 mean)。

//-------------------------Reference-------------------------
// L. Abualigah, M. Abd Elaziz, P. Sumari, Z. W. Geem, A. H. Gandomi, "Reptile Search Algorithm (RSA):
// A nature-inspired meta-heuristic optimizer," Expert Systems with Applications, vol. 191, 116158, 2022,
// doi: 10.1016/j.eswa.2021.116158.

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
    class ReptileEncircling : public LearningStrategy
    {
    private:
        double _alpha;      // 百分比差偏置(敏感参数,标准 0.1)
        double _beta;       // 高位行走精度(敏感参数,标准 0.1)
        double _progress;   // 当代运行进度(preparation_s 存)

    public:
        ReptileEncircling(double alpha = 0.1, double beta = 0.1)
            : _alpha(alpha), _beta(beta), _progress(0.0) {}
        ~ReptileEncircling() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // 需 3:最优/随机/中心
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object == nullptr || learning_object[0] == nullptr) return x;  // 无最优 → 不动
            double P    = (*learning_object[0])[decision_d];
            if (is_empty(P)) return x;
            if (is_empty(x)) x = P;
            double x_r  = learning_object[1] ? (*learning_object[1])[decision_d] : P;   // 随机成员(缺→P)
            double mean = learning_object[2] ? (*learning_object[2])[decision_d] : P;   // 逐维种群均值(缺→P)
            if (is_empty(x_r)) x_r = P;
            if (is_empty(mean)) mean = P;
            double UB = problem_handle->getVariableUpbound(decision_d);
            double LB = problem_handle->getVariableLowbound(decision_d);
            const double eps = 1e-9;

            double Pct = _alpha + (x - mean) / (P * (UB - LB) + eps);        // 百分比差
            double eta = P * Pct;                                            // 狩猎算子
            double R   = (P - x_r) / (P + eps);                              // 缩减函数
            double ES  = 2.0 * ((rand01() < 0.5) ? -1.0 : 1.0) * (1.0 - _progress);   // 进化感(随进度衰减的 ±)
            double r   = rand01();

            if      (_progress <= 0.25) return P * (-eta) * _beta - R * r;   // 高位行走(探索)
            else if (_progress <= 0.50) return P * x_r * ES * r;             // 腹部行走(探索)
            else if (_progress <= 0.75) return P * Pct * r;                  // 狩猎协同(开发)
            else                        return P - eta * eps - R * r;        // 协作(开发)
        }
    };

    inline Registry<LearningStrategy>::Entry reptileEncirclingEntry()
    {
        return { "ReptileEncircling", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"alpha", ParamKind::Real, 0.0, 1.0, false, 0.05, 0.3},
                                 {"beta",  ParamKind::Real, 0.0, 1.0, false, 0.05, 0.3} } }, sizeof(ReptileEncircling),
            [](const double* p) -> LearningStrategy* { return p ? new ReptileEncircling(p[0], p[1]) : new ReptileEncircling(); },
            [](AssertList& L, const double* p) { ReptileEncircling::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ReptileEncircling::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_reptile, LearningStrategy, reptileEncirclingEntry());
}
