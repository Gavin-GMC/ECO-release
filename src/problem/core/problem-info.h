//------------------------Description------------------------
// 问题定义态元信息：供应用门面/UI 查询变量、目标与约束，不触发 compile()。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>

namespace EvoFlow
{
    struct ProblemVariableInfo
    {
        std::string name;
        std::string kind;          // decision / constant
        std::string valueType;     // continuous / discrete / allocation / sequence_...
        int         length = 0;
        double      lowerBound = 0;
        double      upperBound = 0;
        bool        hasBounds = false;
    };

    struct ProblemObjectiveInfo
    {
        std::string              name;
        std::vector<std::string> variables;
        bool                     minimize = true;
        int                      priority = 0;
    };

    struct ProblemConstraintInfo
    {
        std::string              type;
        std::vector<std::string> variables;
        double                   penalty = 0;
        std::vector<std::string> objectives;
    };

    struct ProblemDefinitionInfo
    {
        std::string                         name;
        int                                 dimension = 0;
        std::vector<ProblemVariableInfo>    variables;
        std::vector<ProblemObjectiveInfo>   objectives;
        std::vector<ProblemConstraintInfo>  constraints;
    };
}
