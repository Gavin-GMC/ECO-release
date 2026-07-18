//------------------------Description------------------------
// 烟花算法(FWA)两个策略:
//   ExplosionSpark —— 爆炸火花:好烟花幅度小(精细)、差烟花幅度大(探索);随机选若干维,在 ±A_i 内均匀位移。
//   GaussianSpark  —— 高斯火花:随机选若干维乘以 g~N(1,1),沿过原点射线缩放,增多样性。
//-------------------------Reference-------------------------
// Y. Tan, Y. Zhu, "Fireworks Algorithm for Optimization," ICSI 2010, LNCS 6145, pp. 355-364,
// doi: 10.1007/978-3-642-13495-1_44.
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
    // 爆炸火花:幅度∝适应度差,随机维均匀位移
    class ExplosionSpark : public LearningStrategy
    {
    private:
        double _amp_max;    // Â:最大爆炸幅度
        double _dim_rate;   // 每维被选中(参与位移)的概率
        std::unordered_map<Individual*, double> _amp;   // 只读:每烟花爆炸幅度(preparation_s 建)
        static constexpr double MIN_RATE = 0.05;        // 最小幅度比例(避免最优烟花幅度为 0)

    public:
        ExplosionSpark(double amp_max = 5.0, double dim_rate = 0.5)
            : _amp_max(amp_max), _dim_rate(dim_rate) {}
        ~ExplosionSpark() {}

        static void preAssert(AssertList& list, double*) {}
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); // 逐维构造
        }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            int N = population.getSize();
            _amp.clear();
            if (N <= 0) return;

            int best = 0;
            for (int i = 1; i < N; i++)
                if (population[i] < population[best]) best = i;
            double f_best = population[best].solution.fitness[0];

            std::vector<double> diff(N);
            double sum_d = 0.0;
            for (int i = 0; i < N; i++)
            {
                diff[i] = std::fabs(population[i].solution.fitness[0] - f_best);   // 离最优越远,幅度越大
                sum_d += diff[i];
            }

            double floor_amp = _amp_max * MIN_RATE;
            for (int i = 0; i < N; i++)
            {
                double A = (sum_d > 1e-12) ? _amp_max * diff[i] / sum_d : _amp_max * 0.5;   // 全等则均匀
                if (A < floor_amp) A = floor_amp;   // 下限,保最优烟花仍探索
                _amp[&population[i]] = A;
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            double x = (*individual)[decision_d];
            if (rand01() >= _dim_rate) return x;               // 未选中该维
            auto it = _amp.find(individual);
            double A = (it != _amp.end()) ? it->second : _amp_max * MIN_RATE;
            return x + get_rand_real(-A, A);                    // ±A 内均匀位移
        }
    };

    // 高斯火花:随机维乘以 g~N(1,1)(每火花一个 g,存 FireworkIndividual)
    class GaussianSpark : public LearningStrategy
    {
    private:
        double _dim_rate;   // 每维被选中(参与缩放)的概率
        std::string _fw_key;   // INDIV-COMPOSE:fireworkstate 特性身份键(装配期解析,ini 缓存)

    public:
        // fireworkstate = 每实例私有的 1 标量 g(Private/scalar,k=1),每火花 preparation_i 重算 → 零初值即可
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "fireworkstate", "scalar", FeatureScope::Private, { 1 } } };
        }
        explicit GaussianSpark(double dim_rate = 0.5) : _dim_rate(dim_rate) {}
        ~GaussianSpark() {}

        static void preAssert(AssertList&, double*)
        {
            // fireworkstate 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); // 逐维构造
        }

        void ini(ProblemHandle*) override { _fw_key = featureKey("fireworkstate"); }   // 缓存 fireworkstate 身份键

        // 每火花抽一次 g,存进 child 的 fireworkstate;逐维复用 → constructive-safe(状态随对象走)
        void preparation_i(Individual* individual, Solution**, Individual* child) override
        {
            child->feature<ScalarFeature>(_fw_key)->vals[0] = get_normal(1.0, 1.0);
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            double x = (*individual)[decision_d];
            if (rand01() >= _dim_rate) return x;               // 未选中该维
            double g = child->feature<ScalarFeature>(_fw_key)->vals[0];
            return x * g;                                       // 乘性缩放
        }
    };

    inline Registry<LearningStrategy>::Entry explosionSparkEntry()
    {
        return { "ExplosionSpark", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"amp_max",  ParamKind::Real, 0.0, 1e6, false, 1.0, 10.0},
                                 {"dim_rate", ParamKind::Real, 0.0, 1.0, false, 0.3, 0.7} } }, sizeof(ExplosionSpark),
            [](const double* p) -> LearningStrategy* { return p ? new ExplosionSpark(p[0], p[1]) : new ExplosionSpark(); },
            [](AssertList& L, const double* p) { ExplosionSpark::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { ExplosionSpark::postAssert(L, const_cast<double*>(p)); } };
    }
    inline Registry<LearningStrategy>::Entry gaussianSparkEntry()
    {
        return { "GaussianSpark", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"dim_rate", ParamKind::Real, 0.0, 1.0, false, 0.3, 0.7} } }, sizeof(GaussianSpark),
            [](const double* p) -> LearningStrategy* { return p ? new GaussianSpark(p[0]) : new GaussianSpark(); },
            [](AssertList& L, const double* p) { GaussianSpark::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { GaussianSpark::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_explosionspark, LearningStrategy, explosionSparkEntry());
    ECFLOW_REGISTER(lstrat_gaussianspark,  LearningStrategy, gaussianSparkEntry());
}
