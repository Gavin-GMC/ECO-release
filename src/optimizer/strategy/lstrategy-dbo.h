//------------------------Description------------------------
// 蜣螂优化(Dung Beetle Optimizer)学习策略 DungBeetleRoles:种群按比例分四角色(滚球/繁殖/觅食/偷窃),各自更新。
//   向导来自 DungBeetleRefs(end[0]=全局最优 X^b、end[1]=当代最优 X*、end[2]=当代最差 X_worst)。
//   滚球角色需上一代位置 x(t-1),经 prevpos(SolutionFeature)存(preparation_i 设为父代当前位置)。

//-------------------------Reference-------------------------
// J. Xue, B. Shen, "Dung beetle optimizer: a new meta-heuristic algorithm for global optimization,"
// The Journal of Supercomputing, vol. 79, pp. 7305-7336, 2022, doi: 10.1007/s11227-022-04959-6.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
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
    class DungBeetleRoles : public LearningStrategy
    {
    private:
        double _k, _b, _S;                       // 滚球偏转 k(0.1)/滚球步 b(0.3)/偷窃 S(0.5)(罕调)
        double _obstacle_p;                      // 滚球遇障概率(0.1,罕调)
        double _p_roll, _p_brood, _p_forage;     // 角色比例(滚球/繁殖/觅食;偷窃=余)
        double _R;                               // 收缩 1−p
        std::string _key;                        // prevpos 特性身份键
        std::unordered_map<Individual*, int> _role;   // 只读:每个体角色 0滚球/1繁殖/2觅食/3偷窃

    public:
        DungBeetleRoles(double k = 0.1, double b = 0.3, double S = 0.5, double obstacle_p = 0.1,
                        double p_roll = 0.2, double p_brood = 0.2, double p_forage = 0.233)
            : _k(k), _b(b), _S(S), _obstacle_p(obstacle_p), _p_roll(p_roll), _p_brood(p_brood), _p_forage(p_forage), _R(1.0) {}
        ~DungBeetleRoles() {}

        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "prevpos", "solution", FeatureScope::Private, {} } };   // x(t-1),复用 pbest 同款 solution 特性
        }

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 3, MatchType::notLessButNotice)); // X^b/X*/X_worst
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _key = featureKey("prevpos"); _R = 1.0; _role.clear(); }

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            double p = terminator ? terminator->getProgress() : 0.0;
            _R = 1.0 - p;
            _role.clear();
            int N = population.getSize();
            int n_roll = (int)(_p_roll * N), n_brood = (int)(_p_brood * N), n_forage = (int)(_p_forage * N);
            for (int i = 0; i < N; i++)
            {
                int role = (i < n_roll) ? 0 : (i < n_roll + n_brood) ? 1 : (i < n_roll + n_brood + n_forage) ? 2 : 3;
                _role[&population[i]] = role;
            }
        }

        // preparation_i:父/子直接成对——把父代当前位置存进子代 prevpos(下一代即 x(t-1))。
        void preparation_i(Individual* individual, Solution**, Individual* child) override
        {
            SolutionFeature* f = child->feature<SolutionFeature>(_key);
            if (f) f->sol = individual->solution;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;                     // 无最优 → 不动
            const double PI = 3.14159265358979323846;
            double Xb = (*learning_object[0])[decision_d];                   // 全局最优
            double Xstar = learning_object[1] ? (*learning_object[1])[decision_d] : Xb;  // 当代最优
            double Xworst = learning_object[2] ? (*learning_object[2])[decision_d] : Xb; // 当代最差
            double ub = problem_handle->getVariableUpbound(decision_d);
            double lb = problem_handle->getVariableLowbound(decision_d);
            SolutionFeature* pf = individual->feature<SolutionFeature>(_key);
            double prev = pf ? pf->sol[decision_d] : x;                      // x(t-1)

            auto it = _role.find(individual);
            int role = (it != _role.end()) ? it->second : 3;

            if (role == 0)                                                   // 滚球
            {
                if (rand01() < _obstacle_p)                                 // 遇障:跳舞重定向
                {
                    double theta = rand01() * PI;
                    return x + std::tan(theta) * std::fabs(x - prev);
                }
                double alpha = (rand01() < 0.5) ? 1.0 : -1.0;               // 无障:滚球
                return x + alpha * _k * prev + _b * std::fabs(x - Xworst);
            }
            if (role == 1)                                                   // 繁殖
            {
                double Lbs = std::max(Xstar * (1.0 - _R), lb);
                double Ubs = std::min(Xstar * (1.0 + _R), ub);
                return Xstar + get_normal(0.0, 1.0) * (x - Lbs) + get_normal(0.0, 1.0) * (x - Ubs);
            }
            if (role == 2)                                                   // 觅食
            {
                double Lbb = std::max(Xb * (1.0 - _R), lb);
                double Ubb = std::min(Xb * (1.0 + _R), ub);
                return x + get_normal(0.0, 1.0) * (x - Lbb) + rand01() * (x - Ubb);
            }
            // role == 3 偷窃
            return Xb + _S * get_normal(0.0, 1.0) * (std::fabs(x - Xstar) + std::fabs(x - Xb));
        }
    };

    inline Registry<LearningStrategy>::Entry dungBeetleRolesEntry()
    {
        return { "DungBeetleRoles", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"k",          ParamKind::Real, 0.0, 1.0, false, 0.05, 0.3},   // 罕调
                                 {"b",          ParamKind::Real, 0.0, 1.0, false, 0.1, 0.5},    // 罕调
                                 {"S",          ParamKind::Real, 0.0, 2.0, false, 0.3, 0.7},    // 罕调
                                 {"obstacle_p", ParamKind::Real, 0.0, 1.0, false, 0.05, 0.3},   // 罕调
                                 {"p_roll",     ParamKind::Real, 0.0, 1.0, false, 0.1, 0.3},
                                 {"p_brood",    ParamKind::Real, 0.0, 1.0, false, 0.1, 0.3},
                                 {"p_forage",   ParamKind::Real, 0.0, 1.0, false, 0.15, 0.35} } }, sizeof(DungBeetleRoles),
            [](const double* p) -> LearningStrategy* { return p ? new DungBeetleRoles(p[0], p[1], p[2], p[3], p[4], p[5], p[6]) : new DungBeetleRoles(); },
            [](AssertList& L, const double* p) { DungBeetleRoles::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DungBeetleRoles::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_dbo, LearningStrategy, dungBeetleRolesEntry());
}
