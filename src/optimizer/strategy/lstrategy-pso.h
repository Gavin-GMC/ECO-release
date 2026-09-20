//------------------------Description------------------------
// 速度驱动学习策略 VelocityDrivenStrategy:经速度更新 + 位置更新生成子代(连续空间粒子群机制)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "evoflow-constant.h"
#include "evoflow-basicfunc.h"
#include "solution.h"
#include "individual.h"
#include "learning-strategy.h"
#include "registry.h"
#include "terminator.h"

namespace EvoFlow
{
    class VelocityDrivenStrategy : public LearningStrategy
    {
    private:
        int _object_number;
        double* _c;
        double _w_ini;
        double _w;
        double _w_final;
        double _vel_init_scale;   // velocity 初始化:0→Zero(默认);>0→RandomInDomain,幅度 k=该值(±k·域宽)
        std::string _vel_key;   // INDIV-COMPOSE:velocity 特性的身份键(装配期解析,ini 缓存)

    public:
        // velocity = 每实例私有的按维向量(Private/vector);初始化策略由本策略参数 vel_init_scale 调控
        std::vector<FeatureDemand> featureDemands() const override
        {
            FeatureInit im = (_vel_init_scale > 0.0) ? FeatureInit::RandomInDomain : FeatureInit::Zero;
            return { { "velocity", "vector", FeatureScope::Private, {}, im, _vel_init_scale } };
        }
        VelocityDrivenStrategy(int object_number = 2, double c = 2, double w_ini = 0.9, double w_final = EMPTYVALUE, double vel_init_scale = 0.0)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            for (int i = 0; i < object_number; i++)
                _c[i] = c;
            _w_ini = w_ini;
            _w_final = w_final;
            _vel_init_scale = vel_init_scale;
            _w = 0;
        }

        VelocityDrivenStrategy(int object_number, double* c, double w_ini = 0.9, double w_final = EMPTYVALUE, double vel_init_scale = 0.0)
            : LearningStrategy()
        {
            _object_number = object_number;
            _c = new double[object_number];
            memcpy(_c, c, sizeof(double) * object_number);
            _w_ini = w_ini;
            _w_final = w_final;
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

        void preparation_s(IndividualArray& population, Terminator* terminator) override
        {
            if (!is_empty(_w_final))
            {
                double progress = terminator ? terminator->getProgress() : 0.0;
                if (progress < 0.0) progress = 0.0;
                if (progress > 1.0) progress = 1.0;
                _w = _w_ini + (_w_final - _w_ini) * progress;
            }
        }

        void preparation_i(Individual* individual, Solution** learning_object, Individual* child) override
        {
            std::vector<double>& pvel = individual->feature<VectorFeature>(_vel_key)->data;   // 亲本速度
            std::vector<double>& cvel = child->feature<VectorFeature>(_vel_key)->data;         // 子代速度

            double v;
            int problem_size = individual->getSolutionSize();
            for (int d = 0; d < problem_size; d++)
            {
                double x = (*individual)[d];
                double base = x;
                if (is_empty(base) && learning_object != nullptr)
                    for (int i = 0; i < _object_number; i++)
                        if (learning_object[i] != nullptr && !is_empty((*learning_object[i])[d]))
                        {
                            base = (*learning_object[i])[d];
                            break;
                        }

                if (is_empty(base))
                {
                    cvel[d] = 0.0;
                    continue;
                }

                if (!is_empty(x) && !is_empty(pvel[d]))
                {
                    if (is_empty(_w_final))    // 未设终值 → 保留既有逐维随机惯性
                        v = rand01_() * pvel[d];
                    else
                        v = _w * pvel[d];
                }
                else
                    v = 0.0;

                for (int i = 0; i < _object_number; i++)
                    if (learning_object != nullptr && learning_object[i] != nullptr && !is_empty((*learning_object[i])[d]))
                        v += _c[i] * rand01_() * ((*learning_object[i])[d] - base);

                cvel[d] = v;
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            double base = (*individual)[decision_d];
            if (is_empty(base) && learning_object != nullptr)
                for (int i = 0; i < _object_number; i++)
                    if (learning_object[i] != nullptr && !is_empty((*learning_object[i])[decision_d]))
                    {
                        base = (*learning_object[i])[decision_d];
                        break;
                    }
            if (is_empty(base)) return EMPTYVALUE;

            double velocity = child->feature<VectorFeature>(_vel_key)->data[decision_d];
            if (is_empty(velocity)) velocity = 0.0;
            return base + velocity;
        }
    };

    inline Registry<LearningStrategy>::Entry velocityDrivenStrategyEntry()
    {
        return { "VelocityDriven", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"object_number",  ParamKind::Int,  1, 10,   false, 2, 2},
                                 {"c",              ParamKind::Real, 0.0, 10.0, false, 1.5, 2.5},
                                 {"w_ini",          ParamKind::Real, 0.0, 1.0,  false, 0.4, 0.9},
                                 {"w_final",        ParamKind::Real, 0.0, 1.0,  true,  0.2, 0.9},
                                 {"vel_init_scale", ParamKind::Real, 0.0, 1.0,  false, 0.0, 0.5} } }, sizeof(VelocityDrivenStrategy),  // velocity 随机初始化幅度(**0=Zero,即"关"本身就是合法取值 → 无"未设置"态**,故 allow_empty=false;原声明 true 但 ctor 默认是 0.0 而非 EMPTYVALUE、实现用 `>0.0` 判定,留空传 NaN 时 `NaN>0.0` 为假恰好也落 Zero——靠巧合正确,已订正声明)
            [](const double* p) -> LearningStrategy* {
                return p ? new VelocityDrivenStrategy(int(p[0]), p[1], p[2], p[3], p[4]) : new VelocityDrivenStrategy();
            },
            [](AssertList& L, const double* p) { VelocityDrivenStrategy::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { VelocityDrivenStrategy::postAssert(L, const_cast<double*>(p)); } };
    }
    EVOFLOW_REGISTER(lstrat_velocitydriven, LearningStrategy, velocityDrivenStrategyEntry());
}
