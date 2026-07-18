//------------------------Description------------------------
// 解初始化器:生成初始候选解的不同策略(随机 / 贪心 / 随机-贪心混合 / 分布 / 无)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "problem-handle.h"
#include "distribution-factory.h"
#include "solution.h"

namespace ECFlow
{
    // 解初始化器抽象基类
    class SolutionInitializer
    {
    protected:
        ProblemHandle* _problem_handle;
        bool           _evaluate;
    public:
        SolutionInitializer(bool evaluate)
        {
            _problem_handle = nullptr;
            _evaluate = evaluate;
        }

        virtual ~SolutionInitializer() { delete _problem_handle; }

        virtual void ini_solution(double* solution, const int size) = 0;

        void ini_solution(Solution& solution)
        {
            ini_solution(solution.result, solution.getSolutionSize());

            if (_evaluate)
                _problem_handle->solutionEvaluate(solution);
            else
                solution.fitness[0] = EMPTYVALUE;   // 标记未评估
        }

        virtual void setProblem(ProblemHandle* problem_handle)
        {
            delete _problem_handle;
            _problem_handle = new ProblemHandle(*problem_handle);
        }

        double getRandomValue(int demension_id)
        {
            return _problem_handle->getRandomChoice(demension_id);
        }

        // 透传初始化所用句柄(特性 RandomInDomain 取域边界用;个体不自持句柄)
        ProblemHandle* handle() const { return _problem_handle; }
    };

    // 随机初始化:逐维在可行域内随机取值
    class RandomInitializer final : public SolutionInitializer
    {
    public:
        RandomInitializer() : SolutionInitializer(true) {}
        ~RandomInitializer() {}

        void ini_solution(double* solution, const int size) override
        {
            _problem_handle->constrainReset();
            for (int i = 0; i < size; i++)
            {
                solution[i] = _problem_handle->getRandomChoiceInspace(i);
                _problem_handle->constrainChange(i, solution[i]);
            }
        }
    };

    // 贪心初始化:逐维取优先级最高的可行值
    class GreedyInitializer final : public SolutionInitializer
    {
    public:
        GreedyInitializer() : SolutionInitializer(true) {}
        ~GreedyInitializer() {}

        void ini_solution(double* solution, const int size) override
        {
            _problem_handle->setResult(solution);
            _problem_handle->constrainReset();
            for (int i = 0; i < size; i++)
            {
                solution[i] = _problem_handle->getPrioriChoice(i);
                _problem_handle->constrainChange(i, solution[i]);
            }
        }
    };

    // 按概率的随机-贪心混合初始化
    // [已修复 v3.2]:完成原代码未写完的 WIP —— ① 构造入参 int→double(概率);② 循环前补 setResult
    //   (贪心分支 getPrioriChoice 需读部分解,同 GreedyInitializer);③ 恢复被注释的贪心分支。
    class R_GInitializer final : public SolutionInitializer
    {
    private:
        double _greedy_posibility;
    public:
        R_GInitializer(double posibility) : SolutionInitializer(true)
        {
            _greedy_posibility = posibility;
        }
        ~R_GInitializer() {}

        void ini_solution(double* solution, const int size) override
        {
            _problem_handle->setResult(solution);      // 绑定解:贪心分支 getPrioriChoice 需读部分解
            _problem_handle->constrainReset();
            for (int i = 0; i < size; i++)
            {
                if (rand01() < _greedy_posibility)
                    solution[i] = _problem_handle->getPrioriChoice(i);
                else
                    solution[i] = _problem_handle->getRandomChoiceInspace(i);
                _problem_handle->constrainChange(i, solution[i]);
            }
        }
    };

    // 基于分布模型的初始化:逐维按分布采样,越界则取最近可行值
    class DistributionInitializer final : public SolutionInitializer
    {
    private:
        DistributionType    _model_type;
        int                 _demensions;
        DistributionModel** _models;

        void _deleteModels()
        {
            if (_models == nullptr) return;
            for (int i = 0; i < _demensions; i++) delete _models[i];
            delete[] _models;
            _models = nullptr;
        }

    public:
        // [迁移适配 v3.4.c]:构造只存分布型;problem_size 由问题决定(builder 装配期尚无问题),
        //   故模型数组分配**延到 setProblem**(那时 getProblemSize 才可用)。原构造签名 (type, size) 遂废。
        DistributionInitializer(DistributionType model_type)
            : SolutionInitializer(true)
        {
            _model_type = model_type;
            _models = nullptr;
            _demensions = 0;
        }

        ~DistributionInitializer() { _deleteModels(); }

        void setProblem(ProblemHandle* problem_handle) override
        {
            SolutionInitializer::setProblem(problem_handle);   // 建 _problem_handle 副本
            _deleteModels();
            _demensions = problem_handle->getProblemSize();
            _models = DistributionFactory::newModelArray(_model_type, _demensions);
            // 冷启动无样本:按每维问题域 [lo,hi] 给分布提供参数初值,否则 fresh 模型恒 0(初始化退化成单点)
            if (_models != nullptr)
                for (int i = 0; i < _demensions; i++)
                {
                    double lo = problem_handle->getBoundaryChoice(i, true);
                    double hi = problem_handle->getBoundaryChoice(i, false);
                    if (!is_empty(lo) && !is_empty(hi))
                        _models[i]->iniByDomain(lo, hi);
                }
        }

        void ini_solution(double* solution, const int size) override
        {
            _problem_handle->constrainReset();
            for (int i = 0; i < size; i++)
            {
                solution[i] = _models[i]->getValue();
                if (_problem_handle->constrainCheck(i, solution[i]))
                    solution[i] = _problem_handle->getCloseChoice(i, solution[i]);
                _problem_handle->constrainChange(i, solution[i]);
            }
        }
    };

    // 无初始化(占位,不改动传入解)
    class NoInitializer final : public SolutionInitializer
    {
    public:
        NoInitializer() : SolutionInitializer(false) {}
        ~NoInitializer() {}
        void ini_solution(double* solution, const int size) override {}
    };
}
