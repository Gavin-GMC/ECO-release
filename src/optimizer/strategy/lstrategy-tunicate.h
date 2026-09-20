//------------------------Description------------------------
// 海鞘群喷射(Tunicate Swarm)学习策略 TunicateJetSwarm:海鞘靠喷射推进趋向食物源 FS(当代最优,TopRanked(1)),
//   再与自身当前位置群聚平均。避冲突向量 A=G/M 由随机社会力构成,无进度衰减(TSA 原文即无 t/T)。

//-------------------------Reference-------------------------
// S. Kaur, L. K. Awasthi, A. L. Sangal, G. Dhiman, "Tunicate Swarm Algorithm: A new bio-inspired
// based metaheuristic paradigm for global optimization," Engineering Applications of Artificial
// Intelligence, vol. 90, 103541, 2020, doi: 10.1016/j.engappai.2020.103541.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include "evoflow-rand.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "registry.h"

namespace EvoFlow
{
    class TunicateJetSwarm : public LearningStrategy
    {
    private:
        double _pmin, _pmax;   // 社会力初值/从属速度(标准 1 / 4)

    public:
        TunicateJetSwarm(double pmin = 1.0, double pmax = 4.0) : _pmin(pmin), _pmax(pmax) {}
        ~TunicateJetSwarm() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice)); // 需要 1 个食物源(当代最优)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无食物源 → 不动
            double FS = (*learning_object[0])[decision_d];
            const double eps = 1e-9;
            double c1 = rand01(), c2 = rand01(), c3 = rand01();
            double F = 2.0 * c1;
            double G = c2 + c3 - F;
            double M = std::fabs(_pmin + c1 * (_pmax - _pmin));
            if (M < eps) M = eps;                                            // 防除零(默认参数下恒≥1)
            double A = G / M;                                                // 避冲突向量
            double PD = std::fabs(FS - rand01() * x);                        // 到食物源距离
            double jet = (rand01() >= 0.5) ? (FS + A * PD) : (FS - A * PD);  // 喷射推进
            return (jet + x) / (2.0 + c1);                                   // 群聚平均(与父代当前位置,阻尼)
        }
    };

    inline Registry<LearningStrategy>::Entry tunicateJetSwarmEntry()
    {
        return { "TunicateJetSwarm", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"pmin", ParamKind::Real, 0.0, 10.0, false, 0.5, 2.0},
                                 {"pmax", ParamKind::Real, 0.0, 10.0, false, 2.0, 5.0} } }, sizeof(TunicateJetSwarm),
            [](const double* p) -> LearningStrategy* { return p ? new TunicateJetSwarm(p[0], p[1]) : new TunicateJetSwarm(); },
            [](AssertList& L, const double* p) { TunicateJetSwarm::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { TunicateJetSwarm::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_tunicate, LearningStrategy, tunicateJetSwarmEntry());
}
