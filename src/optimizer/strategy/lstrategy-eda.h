//------------------------Description------------------------
// 分布估计学习策略 DistributionEstimation:统计当前种群优质个体在各维的分布,据此概率模型采样生成子代。
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
#include "learning-strategy.h"
#include "distribution-factory.h"
#include "registry.h"

namespace ECFlow
{
    class DistributionEstimation : public LearningStrategy
    {
    private:
        DistributionType _dm_type;
        DistributionModel** _models;   // 每个维度一个分布模型
        int _demensions;
        double _good_number;           // 未设 = EMPTYVALUE(int→double,以容纳 NaN 哨兵)
        double _good_rate;

        void _deleteModels()
        {
            for (int i = 0; i < _demensions; i++)
                delete _models[i];
            delete[] _models;
        }

    public:
        DistributionEstimation(DistributionType model = DistributionType::F_Gaussian, double good_number = EMPTYVALUE, double good_rate = 0.5)
            : LearningStrategy()
        {
            _dm_type = model;
            _good_number = good_number;
            _good_rate = good_rate;
            _models = nullptr;
            _demensions = 0;
        }

        ~DistributionEstimation() { _deleteModels(); }

        static void preAssert(AssertList& list, double* paras)
        {
            list.add(new Assert(ModuleType::T_learntopology, "objects", 0, MatchType::notLessButNotice)); // 需要 0 个学习目标
        }
        static void postAssert(AssertList& list, double* paras) { list.add(new Assert(ModuleType::T_learnstrategy, "constructive", 1, MatchType::postAssert)); }

        void setProblem(ProblemHandle* problem_handle) override
        {
            if (problem_handle->getProblemSize() != _demensions)
            {
                delete[] _models;
                _demensions = problem_handle->getProblemSize();
                _models = DistributionFactory::newModelArray(_dm_type, _demensions);
            }
        }

        void preparation_s(IndividualArray& population, Terminator*) override
        {
            population.sort();

            for (int i = 0; i < _demensions; i++)
                _models[i]->clear();

            // 未设 good_number → 用 good_rate 比例(修复:原 int==EMPTYVALUE 恒假)
            int good_number = is_empty(_good_number) ? int(_good_rate * population.getSize()) : int(_good_number);

            for (int i = 0; i < _demensions; i++)
            {
                for (int j = 0; j < good_number; j++)
                    _models[i]->addSample(population[j].solution.result[i]);
                _models[i]->build();
            }
        }

        double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) override
        {
            return _models[decision_d]->getValue();
        }
    };

    inline Registry<LearningStrategy>::Entry distributionEstimationEntry()
    {
        return { "DistributionEstimation", ModuleType::T_learnstrategy,
            ParameterTemplate{ { {"model",       ParamKind::Enum, 1, 4,          false, 1, 4},   // DistributionType:1 Gaussian/2 Cauchy/3 Uniform/4 Histogram(F_default=0/F_end=5 无效)
                                 {"good_number", ParamKind::Int,  0, 0x3f3f3f3f, true,  5, 30},
                                 {"good_rate",   ParamKind::Real, 0.0, 1.0,      false, 0.2, 0.5} } }, sizeof(DistributionEstimation),
            [](const double* p) -> LearningStrategy* {
                return p ? new DistributionEstimation(DistributionType(int(p[0])), p[1], p[2])
                         : new DistributionEstimation();
            },
            [](AssertList& L, const double* p) { DistributionEstimation::preAssert(L, const_cast<double*>(p)); },
            [](AssertList& L, const double* p) { DistributionEstimation::postAssert(L, const_cast<double*>(p)); } };
    }
    ECFLOW_REGISTER(lstrat_distributionestimation, LearningStrategy, distributionEstimationEntry());
}
