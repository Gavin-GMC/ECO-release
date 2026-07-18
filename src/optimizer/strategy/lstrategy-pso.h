//------------------------Description------------------------
// 速度驱动学习策略 VelocityDrivenStrategy:经速度更新 + 位置更新生成子代(连续空间粒子群机制)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "ecflow-constant.h"
#include "ecflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "learning-strategy.h"
#include "registry.h"

namespace ECFlow
{
    class VelocityDrivenStrategy : public LearningStrategy
    {
    private:
        int _object_number;
        double* _c;
        double _w_ini;
        double _w;
        double _w_attenuation;
        double _vel_init_scale;   // velocity 初始化:0→Zero(默认);>0→RandomInDomain,幅度 k=该值(±k·域宽)
        std::string _vel_key;   // INDIV-COMPOSE:velocity 特性的身份键(装配期解析,ini 缓存)

    public:
        // velocity = 每实例私有的按维向量(Private/vector);初始化策略由本策略参数 vel_init_scale 调控
        std::vector<FeatureDemand> featureDemands() const override
        {
            FeatureInit im = (_vel_init_scale > 0.0) ? FeatureInit::RandomInDomain : FeatureInit::Zero;
            return { { "velocity", "vector", FeatureScope::Private, {}, im, _vel_init_scale } };
        }
        VelocityDrivenStrategy(int object_number = 2, double c = 2, double w_ini = 0.9, double w_attenuation = EMPTYVALUE, double vel_init_scale = 0.0)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            for (int i = 0; i < object_number; i++)
                _c[i] = c;
            _w_ini = w_ini;
            _w_attenuation = w_attenuation;
            _vel_init_scale = vel_init_scale;
            _w = 0;
        }

        VelocityDrivenStrategy(int object_number, double* c, double w_ini = 0.9, double w_attenuation = EMPTYVALUE, double vel_init_scale = 0.0)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            memcpy(_c, c, sizeof(double) * object_number);
            _w_ini = w_ini;
            _w_attenuation = w_attenuation;
            _vel_init_scale = vel_init_scale;
            _w = 0;
        }

        ~VelocityDrivenStrategy() { delete[] _c; }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", int(paras[0]), MatchType::notLessButNotice)); // 需要学习目标
            // velocity 需求已移入 featureDemands()(INDIV-COMPOSE),不再经个体断言
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void ini(ProblemHandle* problem_handle) override { _w = _w_ini; _vel_key = featureKey("velocity"); }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            if (!is_empty(_w_attenuation))   // 已设衰减(修复:原 != EMPTYVALUE 对 NaN 恒真)
                _w -= _w_attenuation;
        }

        void preparation_i(Individual* individual, Solution** learning_object, Individual* child) override
        {
            std::vector<double>& pvel = individual->feature<VectorFeature>(_vel_key)->data;   // 亲本速度
            std::vector<double>& cvel = child->feature<VectorFeature>(_vel_key)->data;         // 子代速度

            double v;
            int problem_size = individual->getSolutionSize();
            for (int d = 0; d < problem_size; d++)
            {
                if (is_empty(_w_attenuation))    // 未设衰减 → 随机惯性(修复:原 == EMPTYVALUE 对 NaN 恒假,分支曾死)
                    v = rand01_() * pvel[d];
                else
                    v = _w * pvel[d];

                for (int i = 0; i < _object_number; i++)
                    v += _c[i] * rand01_() * ((*learning_object[i])[d] - (*individual)[d]);

                cvel[d] = v;
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            return (*individual)[decision_d] + child->feature<VectorFeature>(_vel_key)->data[decision_d];   // 位置更新
        }
    };

    inline Registry<LearningStrategy>::Entry velocityDrivenStrategyEntry()
    {
        return { "VelocityDriven", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"object_number",  ParamKind::Int,  1, 10,   false, 2, 2},
                                 {"c",              ParamKind::Real, 0.0, 10.0, false, 1.5, 2.5},
                                 {"w_ini",          ParamKind::Real, 0.0, 1.0,  false, 0.4, 0.9},
                                 {"w_attenuation",  ParamKind::Real, 0.0, 1.0,  true,  0.0, 0.1},
                                 {"vel_init_scale", ParamKind::Real, 0.0, 1.0,  false, 0.0, 0.5} } }, sizeof(VelocityDrivenStrategy),  // velocity 随机初始化幅度(**0=Zero,即"关"本身就是合法取值 → 无"未设置"态**,故 allow_empty=false;原声明 true 但 ctor 默认是 0.0 而非 EMPTYVALUE、实现用 `>0.0` 判定,留空传 NaN 时 `NaN>0.0` 为假恰好也落 Zero——靠巧合正确,已订正声明)
            [](const double* p) -> LearningStrategy* {
                return p ? new VelocityDrivenStrategy(int(p[0]), p[1], p[2], p[3], p[4]) : new VelocityDrivenStrategy();
            },
            [](AssertList& L, const double* p) { VelocityDrivenStrategy::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { VelocityDrivenStrategy::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_velocitydriven, LearningStrategy, velocityDrivenStrategyEntry());
}
