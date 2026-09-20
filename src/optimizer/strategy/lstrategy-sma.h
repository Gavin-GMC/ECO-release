//------------------------Description------------------------
// 黏菌振荡(Slime Mould Algorithm)学习策略 SlimeMouldOscillation:黏菌觅食,按适应度权重 W 与振荡系数 vb/vc
//   在最优 X_best 与两随机 X_A/X_B 间振荡逼近或收缩;小概率 z 随机重采样。向导来自 LeaderTwoRandom。

//-------------------------Reference-------------------------
// S. Li, H. Chen, M. Wang, A. A. Heidari, S. Mirjalili, "Slime mould algorithm: A new method for
// stochastic optimization," Future Generation Computer Systems, vol. 111, pp. 300-323, 2020,
// doi: 10.1016/j.future.2020.03.055.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_map>
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
    class SlimeMouldOscillation : public LearningStrategy
    {
    private:
        double _z;          // 随机重采样概率(标准 0.03)
        double _progress;   // 当代进度
        double _a, _b;      // vb/vc 幅度(a=atanh(1-p), b=1-p)
        double _bf;         // 当代最优适应度
        std::unordered_map<Individual*, double> _W;   // 只读:每个体权重(preparation_s 建)

    public:
        explicit SlimeMouldOscillation(double z = 0.03) : _z(z), _progress(0.0), _a(0.0), _b(1.0), _bf(0.0) {}
        ~SlimeMouldOscillation() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // 需 3:最优+两随机
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; _a = 0.0; _b = 1.0; _bf = 0.0; _W.clear(); }

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
            _a = std::atanh(std::min(1.0 - _progress, 1.0 - 1e-9));   // atanh(1)=inf → 上界夹住(progress=0 时 a≈10.6)
            _b = 1.0 - _progress;

            _W.clear();
            int N = population.getSize();
            if (N <= 0) { _bf = 0.0; return; }

            std::vector<int> idx(N);
            for (int i = 0; i < N; i++) idx[i] = i;
            std::sort(idx.begin(), idx.end(), [&](int a, int b) { return population[a] < population[b]; }); // 方向感知,best 在前

            double bF = population[idx[0]].solution.fitness[0];
            double wF = population[idx[N - 1]].solution.fitness[0];
            _bf = bF;
            double denom = bF - wF;
            if (std::fabs(denom) < 1e-12) denom = (denom < 0.0) ? -1e-12 : 1e-12;   // 全等适应度防除零

            for (int rank = 0; rank < N; rank++)
            {
                int i = idx[rank];
                double S = population[i].solution.fitness[0];
                double logterm = std::log(std::fabs((bF - S) / denom) + 1.0);
                double r = rand01();
                _W[&population[i]] = (rank < N / 2) ? (1.0 + r * logterm) : (1.0 - r * logterm);  // 上半 +、下半 −
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无最优 → 不动

            if (rand01() < _z)                                               // 小概率随机重采样
            {
                double UB = problem_handle->getVariableUpbound(decision_d);
                double LB = problem_handle->getVariableLowbound(decision_d);
                return LB + rand01() * (UB - LB);
            }

            double Xbest = (*learning_object[0])[decision_d];
            double XA = learning_object[1] ? (*learning_object[1])[decision_d] : Xbest;
            double XB = learning_object[2] ? (*learning_object[2])[decision_d] : Xbest;
            double S = individual->solution.fitness[0];
            double p = std::tanh(std::fabs(S - _bf));
            double vb = _a * (2.0 * rand01() - 1.0);                         // [−a, a]
            double vc = _b * (2.0 * rand01() - 1.0);                         // [−b, b]
            auto it = _W.find(individual);
            double W = (it != _W.end()) ? it->second : 1.0;

            if (rand01() < p) return Xbest + vb * (W * XA - XB);             // 振荡逼近
            else              return vc * x;                                 // 收缩
        }
    };

    inline Registry<LearningStrategy>::Entry slimeMouldOscillationEntry()
    {
        return { "SlimeMouldOscillation", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"z", ParamKind::Real, 0.0, 1.0, false, 0.01, 0.1} } }, sizeof(SlimeMouldOscillation),
            [](const double* p) -> LearningStrategy* { return p ? new SlimeMouldOscillation(p[0]) : new SlimeMouldOscillation(); },
            [](AssertList& L, const double* p) { SlimeMouldOscillation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { SlimeMouldOscillation::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_sma, LearningStrategy, slimeMouldOscillationEntry());
}
