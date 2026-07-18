//------------------------Description------------------------
// 解修复器:把非法解的某一维修复到合法值(边界 / 随机 / 贪心 / 不修)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <cmath>
#include "solution.h"
#include "ecflow-assert.h"
#include "problem-handle.h"
#include "parameter-template.h"
#include "registry.h"

namespace ECFlow
{
    class SolutionRepaireman
    {
    public:
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        virtual ~SolutionRepaireman() {}

        virtual double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) = 0;
    };

    // 边界修复:小于左界拉到左界,否则拉到右界
    class BoundaryRepair final : public SolutionRepaireman
    {
    public:
        double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) override
        {
            double left = problem_handle->getBoundaryChoice(demensionId);
            if (solution.result[demensionId] < left) return left;
            return problem_handle->getBoundaryChoice(demensionId, false);
        }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
    };

    // 随机修复:取可行域内随机值
    class RandomValueRepair final : public SolutionRepaireman
    {
    public:
        double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) override
        {
            return problem_handle->getRandomChoiceInspace(demensionId);
        }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
    };

    // 贪心修复:取优先级最高的可行值
    class GreedyRepair final : public SolutionRepaireman
    {
    public:
        double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) override
        {
            return problem_handle->getPrioriChoice(demensionId);
        }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
    };

    // 模运算映射修复:越界值绕回域内散布位置 x = L + |x| mod (U-L)(FWA 原文,避免夹边界导致边界堆积)
    class ModularRepair final : public SolutionRepaireman
    {
    public:
        double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) override
        {
            double L = problem_handle->getVariableLowbound(demensionId);
            double U = problem_handle->getVariableUpbound(demensionId);
            double range = U - L;
            if (range <= 0.0) return L;
            return L + std::fmod(std::fabs(solution.result[demensionId]), range);
        }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
    };

    // 不修复:返回原值
    class NoRepair final : public SolutionRepaireman
    {
    public:
        double repair(const Solution& solution, int demensionId, ProblemHandle* problem_handle) override
        {
            return solution.result[demensionId];
        }
        static void preAssert(AssertList& list, double* paras) {}
        static void postAssert(AssertList& list, double* paras) {}
        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }
    };

    // 自注册进 Registry<SolutionRepaireman>(T_Repair),取代原 RepaireFactory 的 F_boundary/F_random/F_greedy/F_no 分派。
    // 均无参、无断言;create 忽略 para。
    inline Registry<SolutionRepaireman>::Entry boundaryRepairEntry()
    {
        return { "Boundary", ModuleType::T_Repair, ParameterTemplate{}, sizeof(BoundaryRepair),
            [](const double*) -> SolutionRepaireman* { return new BoundaryRepair(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SolutionRepaireman>::Entry randomRepairEntry()
    {
        return { "Random", ModuleType::T_Repair, ParameterTemplate{}, sizeof(RandomValueRepair),
            [](const double*) -> SolutionRepaireman* { return new RandomValueRepair(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SolutionRepaireman>::Entry greedyRepairEntry()
    {
        return { "Greedy", ModuleType::T_Repair, ParameterTemplate{}, sizeof(GreedyRepair),
            [](const double*) -> SolutionRepaireman* { return new GreedyRepair(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SolutionRepaireman>::Entry noRepairEntry()
    {
        return { "No", ModuleType::T_Repair, ParameterTemplate{}, sizeof(NoRepair),
            [](const double*) -> SolutionRepaireman* { return new NoRepair(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    inline Registry<SolutionRepaireman>::Entry modularRepairEntry()
    {
        return { "Modular", ModuleType::T_Repair, ParameterTemplate{}, sizeof(ModularRepair),
            [](const double*) -> SolutionRepaireman* { return new ModularRepair(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(rep_boundary, SolutionRepaireman, boundaryRepairEntry());
    ECFLOW_REGISTER(rep_modular,  SolutionRepaireman, modularRepairEntry());
    ECFLOW_REGISTER(rep_random,   SolutionRepaireman, randomRepairEntry());
    ECFLOW_REGISTER(rep_greedy,   SolutionRepaireman, greedyRepairEntry());
    ECFLOW_REGISTER(rep_no,       SolutionRepaireman, noRepairEntry());
}
