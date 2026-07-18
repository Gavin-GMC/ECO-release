//------------------------Description------------------------
// 鲸鱼优化(Whale Optimization Algorithm, WOA)学习策略 WhaleForaging:向领袖鲸(全局最优)包围收缩/螺旋逼近,
//   或以概率向随机鲸探索;系数 a 随运行进度在 [a_min,a_max] 间线性衰减(探索→开发)。配 LeaderAndRandom 拓扑。
//-------------------------Reference-------------------------
// S. Mirjalili, A. Lewis, "The Whale Optimization Algorithm," Advances in Engineering Software,
// vol. 95, pp. 51-67, 2016, doi: 10.1016/j.advengsoft.2016.01.008.
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include "ecflow-rand.h"
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "learning-strategy.h"
#include "terminator.h"
#include "registry.h"

namespace ECFlow
{
    class WhaleForaging : public LearningStrategy
    {
    private:
        double _a_max, _a_min, _b;   // a 衰减边界 + 螺旋常数(全开放,标准 WOA 默认 2/0/1)
        double _a;                   // 当代系数(preparation_s 算好)
        std::string _whale_key;      // INDIV-COMPOSE:whalestate 特性身份键(装配期解析,ini 缓存)

    public:
        // whalestate = 每实例私有的 4 标量 p/A/C/l(Private/scalar,k=4),每代 preparation_i 重算 → 零初值即可
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "whalestate", "scalar", FeatureScope::Private, { 4 } } };
        }
        WhaleForaging(double a_max = 2.0, double a_min = 0.0, double b = 1.0)
            : _a_max(a_max), _a_min(a_min), _b(b), _a(a_max) {}
        ~WhaleForaging() {}

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 领袖 + 随机鲸(2 学习对象)
            // whalestate 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));  // 逐维构造
        }

        void ini(ProblemHandle*) override { _a = _a_max; _whale_key = featureKey("whalestate"); }   // 每轮 exe 复位 + 缓存键

        void preparation_s(IndividualArray&, Terminator* terminator) override
        {
            double progress = terminator ? terminator->getProgress() : 0.0;
            _a = _a_max - (_a_max - _a_min) * progress;
        }

        // 每个体一次:抽 p / A / C / l,存入**父代**个体(offspring_size 无关、ParallelConstruct 批量前置安全)。
        void preparation_i(Individual* individual, Solution**, Individual*) override
        {
            std::vector<double>& w = individual->feature<ScalarFeature>(_whale_key)->vals;   // [0]=p [1]=A [2]=C [3]=l
            double r1 = rand01(), r2 = rand01();
            w[0] = rand01();
            w[1] = 2.0 * _a * r1 - _a;
            w[2] = 2.0 * r2;
            w[3] = 2.0 * rand01() - 1.0;   // l ∈ [-1,1]
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            std::vector<double>& w = individual->feature<ScalarFeature>(_whale_key)->vals;   // [0]=p [1]=A [2]=C [3]=l
            double x = (*individual)[decision_d];
            const double PI = 3.14159265358979323846;

            if (w[0] < 0.5)
            {
                // 包围收缩(|A|<1,向领袖 [0]) / 随机鲸探索(|A|>=1,向随机个体 [1])
                Solution* L = (std::fabs(w[1]) >= 1.0) ? learning_object[1] : learning_object[0];
                if (L == nullptr) L = learning_object[0];   // 随机鲸缺失回退领袖
                if (L == nullptr) return x;                 // 领袖也缺失(空种群)→ 不动
                double Ld = (*L)[decision_d];
                return Ld - w[1] * std::fabs(w[2] * Ld - x);
            }
            else
            {
                // 螺旋气泡网(绕领袖 [0])
                Solution* Lead = learning_object[0];
                if (Lead == nullptr) return x;
                double Ld = (*Lead)[decision_d];
                return std::fabs(Ld - x) * std::exp(_b * w[3]) * std::cos(2.0 * PI * w[3]) + Ld;
            }
        }
    };

    inline Registry<LearningStrategy>::Entry whaleForagingEntry()
    {
        return { "WhaleForaging", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"a_max", ParamKind::Real, 0.0, 10.0, false, 1.0, 3.0},
                                 {"a_min", ParamKind::Real, 0.0, 10.0, false, 0.0, 0.5},
                                 {"b",     ParamKind::Real, 0.0, 10.0, false, 0.5, 2.0} } }, sizeof(WhaleForaging),
            [](const double* p) -> LearningStrategy* { return p ? new WhaleForaging(p[0], p[1], p[2]) : new WhaleForaging(); },
            [](AssertList& L, const double* p) { WhaleForaging::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { WhaleForaging::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_woa, LearningStrategy, whaleForagingEntry());
}
