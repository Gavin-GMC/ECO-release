//------------------------Description------------------------
// 免疫算法(克隆选择)用两个策略:
//   AffinityHypermutation —— 亲和度反比超变异(门控率装饰器):好抗体变异弱、差抗体变异强;逐维以亲和度缩放的
//     变异率门控一个既有变异算子(默认 Gauss)。这是免疫算法的标志特征(亲和度成熟)。
//   RandomGeneration      —— 域内随机重采样(感受器编辑用:产全新随机抗体,忽略父代)。
//-------------------------Reference-------------------------
// L. N. de Castro, F. J. Von Zuben, "Learning and optimization using the clonal selection principle,"
// IEEE Trans. Evolutionary Computation, vol. 6, no. 3, pp. 239-251, 2002, doi: 10.1109/TEVC.2002.1011539.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // 亲和度反比超变异(门控率装饰器):包一个既有变异算子,以亲和度缩放的率逐维门控。
    class AffinityHypermutation : public LearningStrategy
    {
    private:
        LearningStrategy* _inner;                     // 被门控的内部变异(拥有)
        double _base_rate;                            // 最差抗体(aff=0)的变异率
        double _rho;                                  // 亲和度衰减系数(越大,好坏抗体变异率差异越显著)
        std::unordered_map<Individual*, double> _aff; // 只读:每抗体归一亲和度(preparation_s 建,generate 期不变)

    public:
        AffinityHypermutation(LearningStrategy* inner, double base_rate = 0.6, double rho = 4.0)
            : _inner(inner), _base_rate(base_rate), _rho(rho) {}
        ~AffinityHypermutation() { delete _inner; }

        static void preAssert(AssertList& list, double*) {}
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); // 逐维构造
        }

        void ini(ProblemHandle* h) override { if (_inner) _inner->ini(h); _aff.clear(); }
        void setProblem(ProblemHandle* h) override { if (_inner) _inner->setProblem(h); }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            int N = population.getSize();
            _aff.clear();
            std::vector<int> idx(N);
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(),
                      [&population](int a, int b) { return population[a] < population[b]; });   // idx[0]=最优
            for (int r = 0; r < N; r++)
            {
                double aff = (N <= 1) ? 1.0 : (double)(N - 1 - r) / (double)(N - 1);   // 最优=1,最差=0
                _aff[&population[idx[r]]] = aff;
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            auto it = _aff.find(individual);
            double aff = (it != _aff.end()) ? it->second : 0.0;   // 未知 → 视为最差(最强变异),不崩
            double rate = _base_rate * std::exp(-_rho * aff);     // 好抗体率→0(变异弱)
            if (rand01() < rate)
                return _inner->nextDecision(decision_d, individual, learning_object, problem_handle, child);
            return (*individual)[decision_d];                      // 不变
        }
    };

    // 域内随机重采样(感受器编辑:产全新随机抗体)
    class RandomGeneration : public LearningStrategy
    {
    public:
        RandomGeneration() {}
        ~RandomGeneration() {}

        static void preAssert(AssertList& list, double*) {}
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); // 逐维构造
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            return problem_handle->getRandomChoiceInspace(decision_d);   // 域内随机,忽略父代/学习对象
        }
    };

    inline Registry<LearningStrategy>::Entry affinityHypermutationEntry()
    {
        return { "AffinityHypermutation", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"sigma",     ParamKind::Real, 0.0, 10.0, false, 0.1, 1.0},
                                 {"base_rate", ParamKind::Real, 0.0, 1.0,  false, 0.3, 0.8},
                                 {"rho",       ParamKind::Real, 0.0, 20.0, false, 2.0, 6.0} } }, sizeof(AffinityHypermutation),
            [](const double* p) -> LearningStrategy* {
                double sigma     = p ? p[0] : 1.0;
                double base_rate = p ? p[1] : 0.6;
                double rho       = p ? p[2] : 4.0;
                double gp[2] = { sigma, 1.0 };   // 内部 Gauss:率=1(Gauss 无条件扰,由外层门控率控制)
                LearningStrategy* inner = Registry<LearningStrategy>::instance().create("Gauss", gp);
                return new AffinityHypermutation(inner, base_rate, rho);
            },
            [](AssertList& L, const double* p) { AffinityHypermutation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { AffinityHypermutation::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry randomGenerationEntry()
    {
        return { "RandomGeneration", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(RandomGeneration),
            [](const double*) -> LearningStrategy* { return new RandomGeneration(); },
            [](AssertList& L, const double* p) { RandomGeneration::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { RandomGeneration::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_affinityhypermutation, LearningStrategy, affinityHypermutationEntry());
    ECFLOW_REGISTER(lstrat_randomgeneration,      LearningStrategy, randomGenerationEntry());
}
