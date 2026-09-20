//------------------------Description------------------------
// 优化组件基类 O_Component:O_Workflow 流水线的统一单元。每个 stage 的 run(context) 读写共享上下文。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <queue>
#include "solution.h"
#include "individual-array.h"
#include "learning-topology.h"
#include "learning-graph.h"
#include "learning-strategy.h"
#include "problem-handle.h"
#include "best-archive.h"
#include "terminator.h"
#include "solution-repairman.h"

namespace EvoFlow
{
    class O_Component
    {
    protected:
        ProblemHandle* _problem;

    public:
        O_Component() : _problem(nullptr) {}
        virtual ~O_Component() {}

        virtual void ini() {}

        virtual void run(Terminator* terminator_pointer, SolutionRepaireman*& repairman, LearningTopology*& ltopology, LearningStrategy*& lstrategy,
            IndividualArray* offspring, IndividualArray** parent, BestArchive** archive, int subswarm_number,
            std::queue<LearningStrategy*>& after_fes, std::queue<LearningStrategy*>& after_upd, LearningGraph*& last_graph) = 0;

        virtual void setProblem(ProblemHandle* problem_handle) { _problem = problem_handle; }

        // INDIV-COMPOSE(S2):Set*** 包装转发内部算子的特性需求 / 键戳入;其余 no-op。
        virtual std::vector<FeatureDemand> featureDemands() const { return {}; }
        virtual void setFeatureKey(const std::string&, const std::string&) {}
    };
}
