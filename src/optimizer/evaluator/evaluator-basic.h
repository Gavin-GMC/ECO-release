//------------------------Description------------------------
// 基础评估器 BasicEvaluator:对每个未评估子代直接调用真实评估。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "evaluator.h"
#include "registry.h"

namespace ECFlow
{
    class BasicEvaluator final : public Evaluator
    {
    public:
        BasicEvaluator() {}
        ~BasicEvaluator() {}

        void evaluate(Terminator* terminator_pointer, BestArchive* archive_pointer, IndividualArray& offspring) override
        {
            for (int i = 0; i < offspring.getSize(); i++)
            {
                if (offspring[i].has_evaluted)
                    continue;
                if (terminator_pointer->termination())
                    break;

                bool best_solution_updated;
                _problem->solutionEvaluate(offspring[i].solution);
                offspring[i].has_evaluted = true;
                offspring[i].afterEvaluate();   // 评估后特性自更新(pbest 择优保留)
                best_solution_updated = archive_pointer->updateBest(offspring[i].solution);
                terminator_pointer->update(best_solution_updated);
            }
        }
    };

    inline Registry<Evaluator>::Entry basicEvaluatorEntry()
    {
        return { "Basic", ModuleType::T_evaluator, ParameterTemplate{}, sizeof(BasicEvaluator),
            [](const double*) -> Evaluator* { return new BasicEvaluator(); },
            [](AssertList&, const double*) {}, [](AssertList&, const double*) {} };
    }
    ECFLOW_REGISTER(eval_basic, Evaluator, basicEvaluatorEntry());
}
