//------------------------Description------------------------
// 进化策略(ES)学习策略:各向同性高斯扰动 + 全局步长自适应。分布型策略(类 EDA),连续优化。
//   本文件收 ES 各步长自适应变体。
//-------------------------Reference-------------------------
// 新增算子(非迁移)。1/5 规则:Rechenberg 1973。落位参照 lstrategy-eda.h(分布型整群策略)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <vector>
#include "ecflow-constant.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "learning-strategy.h"
#include "ecflow-rand.h"
#include "registry.h"

namespace ECFlow
{
    // ES:1/5 成功规则(Rechenberg)——成功率 > 1/5 增大 σ(探索)、< 1/5 减小 σ(精细)。全局单一 σ。
    class GaussianOneFifth : public LearningStrategy
    {
    private:
        double _sigma;                  // 当前步长
        double _sigma0;                 // 初始步长(ini 复位)
        double _adapt;                  // 调整因子 c(<1);σ/=c 增大、σ*=c 减小
        std::vector<double> _parent_fit;// preparation_s 存父代 fitness(供 update_s 对比)
        int _success, _total;           // 上一代成功数 / 总数

    public:
        GaussianOneFifth(double sigma = 1.0, double adapt = 0.85)
            : _sigma(sigma), _sigma0(sigma), _adapt(adapt), _success(0), _total(0) {}
        ~GaussianOneFifth() {}

        void ini(ProblemHandle*) override { _sigma = _sigma0; _success = 0; _total = 0; }   // 每轮 exe 复位

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 0 学习目标(同 EDA)
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));   // 逐维构造
        }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            // 1/5 规则:按上一代成功率调 σ
            if (_total > 0)
            {
                double ps = (double)_success / _total;
                if (ps > 0.2)      _sigma /= _adapt;   // 成功率高 → 增大 σ
                else if (ps < 0.2) _sigma *= _adapt;   // 成功率低 → 减小 σ
                _success = 0; _total = 0;
            }
            // 存父代 fitness(offspring[i] 由 parent[i] 生成,一一对应)
            int n = population.getSize();
            _parent_fit.assign(n, 0.0);
            for (int i = 0; i < n; i++)
                _parent_fit[i] = population[i].solution.fitness[0];
        }

        double nextDecision(const int decision_d, Individual* individual, Solution**, ProblemHandle*, Individual*) override
        {
            return individual->solution.result[decision_d] + get_normal(0.0, _sigma);   // 父代 + 各向同性高斯
        }

        void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive*) override   // archive 忽略(1/5 用 population)
        {
            int n = offspring.getSize();
            if ((int)_parent_fit.size() < n) n = (int)_parent_fit.size();
            for (int i = 0; i < n; i++)
            {
                _total++;
                if (offspring[i].solution.fitness[0] < _parent_fit[i]) _success++;   // 最小化:更小=成功变异
            }
        }
    };

    inline Registry<LearningStrategy>::Entry gaussianOneFifthEntry()
    {
        return { "GaussianOneFifth", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"sigma", ParamKind::Real, 0.0, 1e9, false, 0.5, 2.0},
                                 {"adapt", ParamKind::Real, 0.0, 1.0, false, 0.8, 0.9} } }, sizeof(GaussianOneFifth),
            [](const double* p) -> LearningStrategy* { return p ? new GaussianOneFifth(p[0], p[1]) : new GaussianOneFifth(); },
            [](AssertList& L, const double* p) { GaussianOneFifth::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { GaussianOneFifth::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_es_onefifth, LearningStrategy, gaussianOneFifthEntry());

    // ES:self-adaptive σ——每维步长随个体遗传+变异+选择(σ 存 StepIndividual)。整体生成(getNewIndividual):
    //   σ'_d = σ_d·exp(τ'·N0 + τ·N_d)(log-normal,N0 全维共享);x'_d = x_d + σ'_d·N_d。学习率 τ'=1/√(2n)、τ=1/√(2√n)。
    //   σ 好的个体被 Rank 选中 → 好 σ 传播(真 self-adaptation)。非 constructive(整体采样),需个体带 sigma(StepIndividual)。
    class GaussianSelfAdapt : public LearningStrategy
    {
    private:
        double _tau_g;   // 全局学习率 τ' = 1/√(2n)
        double _tau_l;   // 每维学习率 τ  = 1/√(2√n)
        std::string _sigma_key;   // INDIV-COMPOSE:sigma 特性身份键(装配期解析,ini 缓存)

    public:
        // sigma = 每实例私有的按维步长向量(Private/vector),初值 σ0=1.0(Constant 策略,个体 ini 时播种)
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "sigma", "vector", FeatureScope::Private, {}, FeatureInit::Constant, 1.0 } };
        }
        GaussianSelfAdapt() : _tau_g(0), _tau_l(0) {}
        ~GaussianSelfAdapt() {}

        void ini(ProblemHandle*) override { _sigma_key = featureKey("sigma"); }   // 缓存 sigma 身份键

        void setProblem(ProblemHandle* problem_handle) override
        {
            int n = problem_handle->getProblemSize();
            if (n < 1) n = 1;
            _tau_g = 1.0 / std::sqrt(2.0 * n);
            _tau_l = 1.0 / std::sqrt(2.0 * std::sqrt((double)n));
        }

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 0 学习目标
            // sigma 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }

        void getNewIndividual(Individual* child, Individual* individual, Solution**, ProblemHandle*) override
        {
            std::vector<double>& csig = child->feature<VectorFeature>(_sigma_key)->data;        // 子代步长
            std::vector<double>& psig = individual->feature<VectorFeature>(_sigma_key)->data;   // 亲代步长
            int n = child->getSolutionSize();
            double g0 = get_normal(0.0, 1.0);   // 全局分量(所有维共享)
            for (int d = 0; d < n; d++)
            {
                csig[d] = psig[d] * std::exp(_tau_g * g0 + _tau_l * get_normal(0.0, 1.0));                  // log-normal σ 变异
                child->solution.result[d] = individual->solution.result[d] + csig[d] * get_normal(0.0, 1.0); // 用新 σ 采样
            }
        }
    };

    inline Registry<LearningStrategy>::Entry gaussianSelfAdaptEntry()
    {
        return { "GaussianSelfAdapt", ModuleType::T_learnstrategy, ParameterTemplate{}, sizeof(GaussianSelfAdapt),
            [](const double*) -> LearningStrategy* { return new GaussianSelfAdapt(); },
            [](AssertList& L, const double* p) { GaussianSelfAdapt::preAssert(L, const_cast<double*>(p)); },
            [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(lstrat_es_selfadapt, LearningStrategy, gaussianSelfAdaptEntry());
}
