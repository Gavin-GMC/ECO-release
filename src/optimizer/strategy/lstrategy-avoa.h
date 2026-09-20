//------------------------Description------------------------
// 非洲秃鹫觅食(African Vultures)学习策略 VultureForaging:两领头秃鹫 Best1/Best2(TopRanked(2)),每秃鹫按概率 L1
//   选一个 R 跟随;饱食率 F 分相位——|F|≥1 探索、0.5≤|F|<1 开发1、|F|<0.5 开发2,各 2 支(含螺旋 / Levy)。

//-------------------------Reference-------------------------
// B. Abdollahzadeh, F. S. Gharehchopogh, S. Mirjalili, "African vultures optimization algorithm: A
// new nature-inspired metaheuristic algorithm for global optimization problems," Computers &
// Industrial Engineering, vol. 158, 107408, 2021, doi: 10.1016/j.cie.2021.107408.

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
#include "strategy-levy.h"
#include "registry.h"

namespace EvoFlow
{
    class VultureForaging : public LearningStrategy
    {
    private:
        double _L1;                 // 选 Best1 的概率(**罕调标准 0.8**;否则选 Best2)
        double _P1, _P2, _P3;       // 三相位内的分支概率(标准 0.6/0.4/0.6)
        double _omega;              // sin 指数(**罕调标准 2.5**)
        double _zb, _hb;            // F 的随机分布界(**罕调标准 z∈[−1,1]、h∈[−2,2]**)
        double _progress;
        std::unordered_map<Individual*, int> _pick;   // 只读:每秃鹫选中的领头 0/1(preparation_s 建)

    public:
        VultureForaging(double L1 = 0.8, double P1 = 0.6, double P2 = 0.4, double P3 = 0.6,
                        double omega = 2.5, double zb = 1.0, double hb = 2.0)
            : _L1(L1), _P1(P1), _P2(P2), _P3(P3), _omega(omega), _zb(zb), _hb(hb), _progress(0.0) {}
        ~VultureForaging() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需 2:两领头秃鹫
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
                _pick[&population[i]] = (rand01() < _L1) ? 0 : 1;   // 概率 L1 选 Best1
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无 Best → 不动
            double Best1 = (*learning_object[0])[decision_d];
            double Best2 = learning_object[1] ? (*learning_object[1])[decision_d] : Best1;
            auto it = _pick.find(individual);
            double R = (it != _pick.end() && it->second == 1) ? Best2 : Best1;

            const double PI = 3.14159265358979323846;
            double z = get_rand_real(-_zb, _zb);
            double h = get_rand_real(-_hb, _hb);
            double t = h * (std::pow(std::sin(PI / 2.0 * _progress), _omega) + std::cos(PI / 2.0 * _progress) - 1.0);
            double F = (2.0 * rand01() + 1.0) * z * (1.0 - _progress) + t;

            if (std::fabs(F) >= 1.0)                                          // 探索
            {
                if (rand01() < _P1)
                    return R - std::fabs(2.0 * rand01() * R - x) * F;
                double UB = problem_handle->getVariableUpbound(decision_d);
                double LB = problem_handle->getVariableLowbound(decision_d);
                return R - F + rand01() * ((UB - LB) * rand01() + LB);
            }
            else if (std::fabs(F) >= 0.5)                                     // 开发 1
            {
                if (rand01() < _P2)
                    return std::fabs(R - x) * (F + rand01()) - (R - x);
                double S1 = R * (rand01() * x / (2.0 * PI)) * std::cos(x);    // 螺旋(角=当前位置)
                double S2 = R * (rand01() * x / (2.0 * PI)) * std::sin(x);
                return R - (S1 + S2);
            }
            else                                                             // 开发 2(|F|<0.5)
            {
                if (rand01() < _P3)
                {
                    const double eps = 1e-9;
                    double d1 = Best1 - x * x; if (std::fabs(d1) < eps) d1 = (d1 < 0.0) ? -eps : eps;
                    double d2 = Best2 - x * x; if (std::fabs(d2) < eps) d2 = (d2 < 0.0) ? -eps : eps;
                    double A1 = Best1 - (Best1 * x) / d1 * F;
                    double A2 = Best2 - (Best2 * x) / d2 * F;
                    return (A1 + A2) / 2.0;
                }
                return R - std::fabs(x) * F * levyFlight();
            }
        }
    };

    inline Registry<LearningStrategy>::Entry vultureForagingEntry()
    {
        return { "VultureForaging", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"L1",    ParamKind::Real, 0.0, 1.0, false, 0.6, 0.9},   // 罕调:选 Best1 概率
                                 {"P1",    ParamKind::Real, 0.0, 1.0, false, 0.4, 0.8},
                                 {"P2",    ParamKind::Real, 0.0, 1.0, false, 0.2, 0.6},
                                 {"P3",    ParamKind::Real, 0.0, 1.0, false, 0.4, 0.8},
                                 {"omega", ParamKind::Real, 0.0, 5.0, false, 1.5, 3.5},   // 罕调:sin 指数
                                 {"zb",    ParamKind::Real, 0.0, 5.0, false, 0.5, 2.0},   // 罕调:z 分布界
                                 {"hb",    ParamKind::Real, 0.0, 5.0, false, 1.0, 3.0} } }, sizeof(VultureForaging),  // 罕调:h 分布界
            [](const double* p) -> LearningStrategy* { return p ? new VultureForaging(p[0], p[1], p[2], p[3], p[4], p[5], p[6]) : new VultureForaging(); },
            [](AssertList& L, const double* p) { VultureForaging::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { VultureForaging::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_avoa, LearningStrategy, vultureForagingEntry());
}
