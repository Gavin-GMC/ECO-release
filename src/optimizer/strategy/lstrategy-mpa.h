//------------------------Description------------------------
// 海洋捕食者运动(Marine Predators)学习策略 MarinePredatorMotion:精英 Elite(=最优,LeaderTwoRandom 的 end[0])
//   引导猎物(个体自身),Brownian(RB=N(0,1))/Levy(RL) 运动按进度分 3 相位;末加 FADs 涡流扰动(忠实:边界跳 + 双随机猎物差)。

//-------------------------Reference-------------------------
// A. Faramarzi, M. Heidarinejad, S. Mirjalili, A. H. Gandomi, "Marine Predators Algorithm: A
// nature-inspired metaheuristic," Expert Systems with Applications, vol. 152, 113377, 2020,
// doi: 10.1016/j.eswa.2020.113377.

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
    class MarinePredatorMotion : public LearningStrategy
    {
    private:
        double _P;          // 步长系数(标准 0.5)
        double _fads;       // FADs 涡流概率(标准 0.2)
        double _progress;   // 当代进度
        double _cf;         // 步控 CF=(1-progress)^(2·progress)

    public:
        MarinePredatorMotion(double P = 0.5, double fads = 0.2) : _P(P), _fads(fads), _progress(0.0), _cf(1.0) {}
        ~MarinePredatorMotion() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // 需 3:Elite + 两随机猎物
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; _cf = 1.0; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
            _cf = std::pow(1.0 - _progress, 2.0 * _progress);   // (1-p)^(2p);p=0→1
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无精英 → 不动
            double Elite = (*learning_object[0])[decision_d];
            double R = rand01();
            double newv;

            // —— 相位运动 ——
            if (_progress < 1.0 / 3.0)                                        // Brownian 探索
            {
                double RB = get_normal(0.0, 1.0);
                newv = x + _P * R * (RB * (Elite - RB * x));
            }
            else if (_progress < 2.0 / 3.0)                                   // 过渡(逐维掷币)
            {
                if (rand01() < 0.5)                                          // Levy 半:开发
                {
                    double RL = levyFlight();
                    newv = x + _P * R * (RL * (Elite - RL * x));
                }
                else                                                        // Brownian 半:探索
                {
                    double RB = get_normal(0.0, 1.0);
                    newv = Elite + _P * _cf * (RB * (RB * Elite - x));
                }
            }
            else                                                             // Levy 开发
            {
                double RL = levyFlight();
                newv = Elite + _P * _cf * (RL * (RL * Elite - x));
            }

            // —— FADs 涡流扰动(忠实两分支,逐维)——
            if (rand01() < _fads)                                            // (a) 边界跳
            {
                double UB = problem_handle->getVariableUpbound(decision_d);
                double LB = problem_handle->getVariableLowbound(decision_d);
                double U = (rand01() < 0.5) ? 1.0 : 0.0;                     // 二值向量分量
                newv += _cf * (LB + rand01() * (UB - LB)) * U;
            }
            else                                                             // (b) 双随机猎物差
            {
                double pr1 = learning_object[1] ? (*learning_object[1])[decision_d] : Elite;
                double pr2 = learning_object[2] ? (*learning_object[2])[decision_d] : Elite;
                double r = rand01();
                newv += (_fads * (1.0 - r) + r) * (pr1 - pr2);
            }
            return newv;
        }
    };

    inline Registry<LearningStrategy>::Entry marinePredatorMotionEntry()
    {
        return { "MarinePredatorMotion", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"P",    ParamKind::Real, 0.0, 2.0, false, 0.3, 0.7},
                                 {"fads", ParamKind::Real, 0.0, 1.0, false, 0.1, 0.3} } }, sizeof(MarinePredatorMotion),
            [](const double* p) -> LearningStrategy* { return p ? new MarinePredatorMotion(p[0], p[1]) : new MarinePredatorMotion(); },
            [](AssertList& L, const double* p) { MarinePredatorMotion::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { MarinePredatorMotion::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_mpa, LearningStrategy, marinePredatorMotionEntry());
}
