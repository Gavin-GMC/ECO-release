//------------------------Description------------------------
// 水波类三算子(源自 Water Wave Optimization):WavePropagation(传播)/ WaveRefraction(折射)/ WaveBreaking(碎波)。
//   每个体两个状态:波高 h(停滞倒计数)与波长 λ(搜索幅度),同存一个原始 scalar 特性 "wavestate"(k=2)。
//-------------------------Reference-------------------------
// Zheng, Y.-J. "Water wave optimization: A new nature-inspired metaheuristic."
//   Computers & Operations Research, 55:1-11, 2015.
// 忠实原文:
//   传播:x'(d) = x(d) + rand(−1,1)·λ·L(d)         (L(d)=该维域宽)
//   波高:传播改进 → h 重置 h_max;未改进 → h−1
//   波长(每代全群):λ = λ·α^( −(f−f_min+ε)/(f_max−f_min+ε) )
//   折射(h 降到 0):x'(d) = N( (x*(d)+x(d))/2 , |x*(d)−x(d)|/2 ),随后 h 重置、λ' = λ·f(x)/f(x')
//   碎波(传播出新最优):随机维 d 生成孤立波 x'(d) = x(d) + N(0,1)·β·L(d),优则替换;β 随进度 β_max→β_min
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
#include <string>
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "ecflow-constant.h"
#include "registry.h"

namespace ECFlow
{
    // wavestate 索引:[0]=h 波高、[1]=λ 波长
    namespace wwo { const int H = 0; const int LAMBDA = 1; const double EPS = 1e-12; }

    // 传播:x' = x + rand(−1,1)·λ·L;全群波长衰减(preparation_s);波高维护(update_s,依选择后是否改进)
    class WavePropagation : public LearningStrategy
    {
    private:
        double _h_max, _alpha, _lambda_init;
        Comparer* _comparer;
        std::vector<double> _snapshot;   // 每代快照(preparation_s 存 → update_s 比);每代一次,非逐个体,成员安全

    public:
        WavePropagation(double h_max = 12.0, double alpha = 1.026, double lambda_init = 0.5)
            : _h_max(h_max), _alpha(alpha), _lambda_init(lambda_init), _comparer(nullptr) {}
        ~WavePropagation() {}

        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "wavestate", "scalar", FeatureScope::Singular, { 2 } } };   // [h, λ]
        }

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));
        }

        void setProblem(ProblemHandle* problem_handle) override { _comparer = problem_handle->getSolutionComparer(); }
        void ini(ProblemHandle*) override { _snapshot.clear(); }

        // 每代:惰性初始化 + 快照 + 全群波长衰减
        void preparation_s(IndividualArray& population, Terminator*) override
        {
            int n = population.getSize();
            _snapshot.assign(n, 0.0);
            double fmin = ECFLOW_MAX, fmax = -ECFLOW_MAX;

            for (int i = 0; i < n; i++)
            {
                ScalarFeature* w = population[i].feature<ScalarFeature>(featureKey("wavestate"));
                if (w->vals[wwo::LAMBDA] <= 0.0) { w->vals[wwo::H] = _h_max; w->vals[wwo::LAMBDA] = _lambda_init; }   // 惰性初始化
                double f = population[i].solution.fitness[0];
                _snapshot[i] = f;
                if (f < fmin) fmin = f;
                if (f > fmax) fmax = f;
            }
            for (int i = 0; i < n; i++)   // λ = λ·α^(−(f−f_min+ε)/(f_max−f_min+ε))
            {
                ScalarFeature* w = population[i].feature<ScalarFeature>(featureKey("wavestate"));
                double e = (_snapshot[i] - fmin + wwo::EPS) / (fmax - fmin + wwo::EPS);
                w->vals[wwo::LAMBDA] *= std::pow(_alpha, -e);
            }
        }

        // 状态跟着波走:把亲代 wavestate 拷给子代(原始 scalar 不随出生继承,见头注)
        void preparation_i(Individual* individual, Solution**, Individual* child) override
        {
            if (child == individual) return;
            ScalarFeature* p = individual->feature<ScalarFeature>(featureKey("wavestate"));
            ScalarFeature* c = child->feature<ScalarFeature>(featureKey("wavestate"));
            c->vals = p->vals;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution**,
                            ProblemHandle* problem_handle, Individual*) override
        {
            ScalarFeature* w = individual->feature<ScalarFeature>(featureKey("wavestate"));
            double L = problem_handle->getVariableUpbound(decision_d) - problem_handle->getVariableLowbound(decision_d);
            return (*individual)[decision_d] + (2.0 * rand01() - 1.0) * w->vals[wwo::LAMBDA] * L;   // rand(−1,1)·λ·L
        }

        // 选择之后:改进 → h 重置;未改进 → h−1
        void update_s(IndividualArray& population, IndividualArray&, BestArchive*) override
        {
            int n = population.getSize();
            if ((int)_snapshot.size() < n) return;
            for (int i = 0; i < n; i++)
            {
                ScalarFeature* w = population[i].feature<ScalarFeature>(featureKey("wavestate"));
                bool improved = _comparer && _comparer->isBetter(population[i].solution.fitness, &_snapshot[i]);
                if (improved) w->vals[wwo::H] = _h_max;
                else          w->vals[wwo::H] -= 1.0;
            }
        }
    };

    // 折射:x' = N( (x*+x)/2 , |x*−x|/2 );子代 h 重置;λ' = λ·f(x)/f(x')(update_s 里,新旧同时可见)
    class WaveRefraction : public LearningStrategy
    {
    private:
        double _h_max;
    public:
        explicit WaveRefraction(double h_max = 12.0) : _h_max(h_max) {}
        ~WaveRefraction() {}

        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "wavestate", "scalar", FeatureScope::Singular, { 2 } } };
        }

        static void preAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 1, MatchType::notLessButNotice));   // 需 x*(最优)
        }
        static void postAssert(AssertList& list, double* /*paras*/)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));
        }

        // 折射后 h 重置 h_max(写子代;λ 先随亲代带过来,待 update_s 按 f(x)/f(x') 更新)
        void preparation_i(Individual* individual, Solution**, Individual* child) override
        {
            if (child == individual) return;
            ScalarFeature* p = individual->feature<ScalarFeature>(featureKey("wavestate"));
            ScalarFeature* c = child->feature<ScalarFeature>(featureKey("wavestate"));
            c->vals = p->vals;
            c->vals[wwo::H] = _h_max;
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object,
                            ProblemHandle*, Individual*) override
        {
            double x = (*individual)[decision_d];
            if (learning_object[0] == nullptr) return x;
            double xb = (*learning_object[0])[decision_d];                       // x* = 档案最优
            double mu = (xb + x) / 2.0;                                          // 均值
            double sd = std::fabs(xb - x) / 2.0;                                 // 标准差
            return get_normal(mu, sd);
        }

        // 选择(无条件替换)之后:population[i]=折射解 f(x')、offspring[i]=旧解 f(x) → λ' = λ·f(x)/f(x')
        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive*) override
        {
            int n = population.getSize();
            if (offspring.getSize() < n) return;
            for (int i = 0; i < n; i++)
            {
                double fnew = population[i].solution.fitness[0];
                double fold = offspring[i].solution.fitness[0];
                if (!(std::fabs(fnew) > wwo::EPS)) continue;                     // 防除零
                ScalarFeature* w = population[i].feature<ScalarFeature>(featureKey("wavestate"));
                w->vals[wwo::LAMBDA] *= (fold / fnew);
            }
        }
    };

    // 碎波:孤立波 —— 随机 1 维 x'(d) = x(d) + N(0,1)·β·L(d);β 随进度 β_max→β_min 衰减
    class WaveBreaking : public LearningStrategy
    {
    private:
        double _beta_max, _beta_min, _beta;
    public:
        WaveBreaking(double beta_max = 0.25, double beta_min = 0.001)
            : _beta_max(beta_max), _beta_min(beta_min), _beta(beta_max) {}
        ~WaveBreaking() {}

        static void preAssert(AssertList& /*list*/, double* /*paras*/) {}
        static void postAssert(AssertList& /*list*/, double* /*paras*/) {}

        void ini(ProblemHandle*) override { _beta = _beta_max; }

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _beta = _beta_max - (_beta_max - _beta_min) * progress;
        }

        // 一个孤立波 = 复制 x* 后只扰动随机 1 维(整体型:override getNewIndividual)
        void getNewIndividual(Individual* child, Individual* individual, Solution**, ProblemHandle* problem_handle) override
        {
            int n = child->getSolutionSize();
            for (int d = 0; d < n; d++) child->solution.result[d] = (*individual)[d];
            int d = get_int(0, n - 1);
            double L = problem_handle->getVariableUpbound(d) - problem_handle->getVariableLowbound(d);
            child->solution.result[d] = (*individual)[d] + get_normal(0.0, 1.0) * _beta * L;
        }
    };

    inline Registry<LearningStrategy>::Entry wavePropagationEntry()
    {
        return { "WavePropagation", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"h_max",       ParamKind::Int,  1, 0x3f3f3f3f, false, 6, 12},
                                 {"alpha",       ParamKind::Real, 1.0, 2.0,      false, 1.01, 1.05},
                                 {"lambda_init", ParamKind::Real, 0.0, 1e9,      false, 0.3, 0.5} } }, sizeof(WavePropagation),
            [](const double* p) -> LearningStrategy* { return new WavePropagation(p ? p[0] : 12.0, p ? p[1] : 1.026, p ? p[2] : 0.5); },
            [](AssertList& L, const double* p) { WavePropagation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WavePropagation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_wavepropagation, LearningStrategy, wavePropagationEntry());

    inline Registry<LearningStrategy>::Entry waveRefractionEntry()
    {
        return { "WaveRefraction", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"h_max", ParamKind::Int, 1, 0x3f3f3f3f, false, 6, 12} } }, sizeof(WaveRefraction),
            [](const double* p) -> LearningStrategy* { return new WaveRefraction(p ? p[0] : 12.0); },
            [](AssertList& L, const double* p) { WaveRefraction::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WaveRefraction::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_waverefraction, LearningStrategy, waveRefractionEntry());

    inline Registry<LearningStrategy>::Entry waveBreakingEntry()
    {
        return { "WaveBreaking", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"beta_max", ParamKind::Real, 0.0, 1.0, false, 0.2, 0.3},
                                 {"beta_min", ParamKind::Real, 0.0, 1.0, false, 0.001, 0.01} } }, sizeof(WaveBreaking),
            [](const double* p) -> LearningStrategy* { return new WaveBreaking(p ? p[0] : 0.25, p ? p[1] : 0.001); },
            [](AssertList& L, const double* p) { WaveBreaking::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WaveBreaking::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_wavebreaking, LearningStrategy, waveBreakingEntry());
}
