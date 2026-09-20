//------------------------Description------------------------
// 平衡优化(Equilibrium Optimizer)学习策略 EquilibriumUpdate:控制体积质量平衡模型,浓度 C(=个体位置)向随机选中的
//   平衡候选 Ceq 更新。平衡池 = 当代前 4 最优(TopRanked(4)) + 其均值(共 5),每个体随机取一个 Ceq。

//-------------------------Reference-------------------------
// A. Faramarzi, M. Heidarinejad, B. Stephens, S. Mirjalili, "Equilibrium optimizer: A novel
// optimization algorithm," Knowledge-Based Systems, vol. 191, 105190, 2020,
// doi: 10.1016/j.knosys.2019.105190.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
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
    class EquilibriumUpdate : public LearningStrategy
    {
    private:
        double _a1;     // 探索系数(标准 2)
        double _a2;     // 开发系数(**罕调标准 1**)
        double _GP;     // 生成概率(**罕调标准 0.5**)
        double _V;      // 控制体积归一化(**罕调标准 1**;与 λ 耦合,缩放生成项 G/(λ·V))
        double _progress;
        std::unordered_map<Individual*, int> _pick;   // 只读:每个体的平衡池索引 0..4(preparation_s 建)

    public:
        EquilibriumUpdate(double a1 = 2.0, double a2 = 1.0, double GP = 0.5, double V = 1.0)
            : _a1(a1), _a2(a2), _GP(GP), _V(V), _progress(0.0) {}
        ~EquilibriumUpdate() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 4, MatchType::notLessButNotice)); // 需 4:平衡池前 4
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _progress = 0.0; _pick.clear(); }

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            _progress = terminator ? terminator->getProgress() : 0.0;
            _pick.clear();
            int N = population.getSize();
            for (int i = 0; i < N; i++)
                _pick[&population[i]] = EvoFlow::get_int(0, 4);   // 5 选 1(4 候选 + 均值)
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无最优 → 不动

            // 前 4 均值(池第 5 成员;缺项跳过)
            double sum = 0.0; int cnt = 0;
            for (int i = 0; i < 4; i++)
                if (learning_object[i]) { sum += (*learning_object[i])[decision_d]; cnt++; }
            double avg = (cnt > 0) ? (sum / cnt) : x;

            // 选中的 Ceq
            auto it = _pick.find(individual);
            int pick = (it != _pick.end()) ? it->second : 4;
            double Ceq = (pick < 4 && learning_object[pick]) ? (*learning_object[pick])[decision_d] : avg;

            double lambda = rand01();
            double r = rand01();
            double t = std::pow(1.0 - _progress, _a2 * _progress);
            double F = _a1 * ((r < 0.5) ? -1.0 : 1.0) * (std::exp(-lambda * t) - 1.0);
            double GCP = (rand01() >= _GP) ? (0.5 * rand01()) : 0.0;
            double G = GCP * (Ceq - lambda * x) * F;
            double denom = lambda * _V;
            if (std::fabs(denom) < 1e-9) denom = (denom < 0.0) ? -1e-9 : 1e-9;   // λ≈0 防除零
            return Ceq + (x - Ceq) * F + (G / denom) * (1.0 - F);
        }
    };

    inline Registry<LearningStrategy>::Entry equilibriumUpdateEntry()
    {
        return { "EquilibriumUpdate", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"a1", ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0},
                                 {"a2", ParamKind::Real, 0.0, 5.0, false, 0.5, 1.5},   // 罕调
                                 {"GP", ParamKind::Real, 0.0, 1.0, false, 0.3, 0.7},   // 罕调
                                 {"V",  ParamKind::Real, 0.01, 10.0, false, 0.5, 2.0} } }, sizeof(EquilibriumUpdate),  // 罕调:体积归一化
            [](const double* p) -> LearningStrategy* { return p ? new EquilibriumUpdate(p[0], p[1], p[2], p[3]) : new EquilibriumUpdate(); },
            [](AssertList& L, const double* p) { EquilibriumUpdate::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { EquilibriumUpdate::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_eo, LearningStrategy, equilibriumUpdateEntry());
}
