//------------------------Description------------------------
// 评估器 Evaluator:对子代群体逐个求真实适应度,并驱动最优档案与终止器(FES)更新。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "problem-handle.h"
#include "individual.h"
#include "individual-array.h"
#include "terminator.h"
#include "best-archive.h"

namespace ECFlow
{
    class Evaluator
    {
    protected:
        ProblemHandle* _problem;

    public:
        Evaluator() : _problem(nullptr) {}

        virtual ~Evaluator() {}

        virtual void setProblem(ProblemHandle* problem_handle) { _problem = problem_handle; }

        // 对整个子代群体评估(原 run() 的核心动作;队列编排由 v3.7 的 RunEvaluate 包装负责)
        virtual void evaluate(Terminator* terminator_pointer, BestArchive* archive_pointer, IndividualArray& offspring) = 0;
    };
}
