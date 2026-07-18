//------------------------Description------------------------
// 二进制 PSO(BPSO)族学习策略:①BinaryVelocityDriven(标准 sigmoid BPSO:速度更新 + sigmoid 概率取 0/1);
//   ②StickyBinary(粘性 BPSO:stickiness + 直接翻转概率,取代 velocity+sigmoid,复用 velocity 存 stickiness;见文末)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include <algorithm>
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "learning-strategy.h"
#include "registry.h"

namespace ECFlow
{
    class BinaryVelocityDrivenStrategy : public LearningStrategy
    {
    private:
        double _c1, _c2;
        double _w, _r1, _r2;
        double _w_attenuation;
        std::string _vel_key;   // INDIV-COMPOSE:velocity 特性身份键(装配期解析,ini 缓存)

    public:
        // velocity = 每实例私有的按维向量(Private/vector,零初值);读写亲代速度
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "velocity", "vector", FeatureScope::Private, {} } };
        }
        BinaryVelocityDrivenStrategy(double c1 = 2, double c2 = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE)
            : LearningStrategy()
        {
            _c1 = c1;
            _c2 = c2;
            _w = w_ini;
            _w_attenuation = w_attenuation;
        }

        ~BinaryVelocityDrivenStrategy() {}

        void ini(ProblemHandle*) override { _vel_key = featureKey("velocity"); }   // 缓存 velocity 身份键

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // 需要 2 个学习目标
            // velocity 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            if (is_empty(_w_attenuation))    // 未设衰减 → 随机惯性(修复:原 == EMPTYVALUE 恒假)
                _w = rand01_();
            else
                _w -= _w_attenuation;        // 修复笔误:原为 `_w -= EMPTYVALUE`(-= NaN)
            _r1 = rand01_();
            _r2 = rand01_();
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            std::vector<double>& vel = individual->feature<VectorFeature>(_vel_key)->data;   // 亲代速度(逐维读写)
            double x = (*individual)[decision_d];

            // 速度更新(作用于亲代 velocity,忠实保留)
            vel[decision_d] = _w * vel[decision_d] +
                _c1 * _r1 * ((*learning_object[0])[decision_d] - x) +
                _c2 * _r2 * ((*learning_object[1])[decision_d] - x);

            // 位置更新(sigmoid 概率取 0/1)
            if (rand01() < sigmoid(vel[decision_d]))
                return 1;
            else
                return 0;
        }
    };

    inline Registry<LearningStrategy>::Entry binaryVelocityDrivenStrategyEntry()
    {
        return { "BinaryVelocityDriven", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"c1",            ParamKind::Real, 0.0, 10.0, false, 1.5, 2.5},
                                 {"c2",            ParamKind::Real, 0.0, 10.0, false, 1.5, 2.5},
                                 {"w_ini",         ParamKind::Real, 0.0, 1.0,  false, 0.4, 0.9},
                                 {"w_attenuation", ParamKind::Real, 0.0, 1.0,  true,  0.0, 0.1} } }, sizeof(BinaryVelocityDrivenStrategy),
            [](const double* p) -> LearningStrategy* {
                return p ? new BinaryVelocityDrivenStrategy(p[0], p[1], p[2], p[3]) : new BinaryVelocityDrivenStrategy();
            },
            [](AssertList& L, const double* p) { BinaryVelocityDrivenStrategy::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { BinaryVelocityDrivenStrategy::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_binaryvelocitydriven, LearningStrategy, binaryVelocityDrivenStrategyEntry());

    // ============ Sticky BPSO(粘性二进制 PSO):以粘性 stickiness([0,1] 有界 momentum)+ 直接翻转概率取代 velocity+sigmoid ============
    // Nguyen/Xue/Andreae/Zhang(张孟杰) IEEE TEVC 2020;参照用户实现(参考资料/sbpso/sched_sdl_hpso.h)提炼。3-term 通用版。
    //   翻转概率 = i_s·(1-s) + i_p·|lo0-x| + i_g·|lo1-x|;**i_s=c/N**(规模自适应)、i_p:i_g 按整数比例分 (1-i_s);
    //   粘性翻转→置 1、否则每代线性衰减 1/ustkS。**复用 Particle.velocity 存每维 stickiness**(初值 0);2 学习目标(objects=2)。
    //   ustkS 直接作参数(论文 8·maxfes/pop/100 手算;策略拿不到 terminator → STICKY-USTKS)。用户原版为 4-term(调度专用 hub 项)。
    class StickyBinary : public LearningStrategy
    {
    private:
        int    _c;              // i_s 分子(i_s = c/N)
        int    _rp, _rg;        // i_p:i_g 的整数比例
        double _ustkS;          // 粘性衰减步数(不翻则每代 s -= 1/ustkS)
        double _is, _ip, _ig;   // setProblem 时按 N 与比例算定
        std::string _vel_key;   // INDIV-COMPOSE:velocity 特性身份键(复用存 stickiness)

    public:
        // velocity = 每实例私有的按维向量(Private/vector,零初值);复用存每维 stickiness
        std::vector<FeatureDemand> featureDemands() const override
        {
            return { { "velocity", "vector", FeatureScope::Private, {} } };
        }
        StickyBinary(int c = 4, int ratio_p = 1, int ratio_g = 1, double ustkS = 20.0)
            : _c(c), _rp(ratio_p), _rg(ratio_g), _ustkS(ustkS), _is(0), _ip(0), _ig(0) {}
        ~StickyBinary() {}

        void ini(ProblemHandle*) override { _vel_key = featureKey("velocity"); }   // 缓存 velocity 身份键

        void setProblem(ProblemHandle* problem_handle) override
        {
            int n = problem_handle->getProblemSize(); if (n < 1) n = 1;
            _is = (double)_c / n; if (_is > 1.0) _is = 1.0;
            double rest = 1.0 - _is;
            int rsum = _rp + _rg; if (rsum < 1) rsum = 1;
            _ip = rest * _rp / rsum;
            _ig = rest * _rg / rsum;
        }

        static void preAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 2, MatchType::notLessButNotice)); // pbest+gbest(或 SDL 对)
            // velocity(存 stickiness)需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }
        static void postAssert(AssertList& list, double*)
        {
            list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert));   // 逐维构造
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle*, Individual*) override
        {
            std::vector<double>& vel = individual->feature<VectorFeature>(_vel_key)->data;   // 复用 velocity 存 stickiness
            double x = (*individual)[decision_d];
            double s = vel[decision_d];
            double lo0 = (*learning_object[0])[decision_d];
            double lo1 = (*learning_object[1])[decision_d];

            // 翻转概率 = i_s·(1-s) + i_p·|lo0-x| + i_g·|lo1-x|
            double flip = _is * (1.0 - s) + _ip * std::fabs(lo0 - x) + _ig * std::fabs(lo1 - x);
            if (flip < 0.0) flip = 0.0; else if (flip > 1.0) flip = 1.0;

            double nx;
            if (rand01() < flip) { nx = 1.0 - x; s = 1.0; }                          // 翻转 → 粘性置 1
            else                 { nx = x; s = std::max(0.0, s - 1.0 / _ustkS); }    // 不翻 → 粘性衰减
            vel[decision_d] = s;
            return nx;
        }
    };

    inline Registry<LearningStrategy>::Entry stickyBinaryEntry()
    {
        return { "StickyBinary", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"c",       ParamKind::Int,  1, 100,   false, 4, 4},    // i_s = c/N
                                 {"ratio_p", ParamKind::Int,  0, 100,   false, 1, 1},    // i_p 比例
                                 {"ratio_g", ParamKind::Int,  0, 100,   false, 1, 1},    // i_g 比例
                                 {"ustkS",   ParamKind::Real, 1.0, 1e4, false, 10, 40} } }, sizeof(StickyBinary),
            [](const double* p) -> LearningStrategy* {
                return p ? new StickyBinary((int)p[0], (int)p[1], (int)p[2], p[3]) : new StickyBinary();
            },
            [](AssertList& L, const double* p) { StickyBinary::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { StickyBinary::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_stickybinary, LearningStrategy, stickyBinaryEntry());
}
