//------------------------Description------------------------
// 学习策略 LearningStrategy:个体如何向学习对象学习、生成子代。定义两条生成路径——
//   整体生成(getNewIndividual / preparation_i)与逐维构造(preparation_d + nextDecision)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <stdexcept>
#include <map>
#include <string>
#include <vector>
#include "solution.h"
#include "individual.h"
#include "individual-array.h"
#include "problem-handle.h"
#include "parameter-template.h"

namespace ECFlow
{
    class BestArchive;   // update_s 传全局最优档案(MMAS 经 getElite 取 global-best);前向声明,基类不 include
    class Terminator;    // preparation_s 传运行总进度(GWO/WOA 的系数 a);前向声明,基类不 include

    class LearningStrategy
    {
    public:
        LearningStrategy() {}

        virtual ~LearningStrategy() {}

        // ---- INDIV-COMPOSE 特性感知(S2):声明特性需求 + 运行期按 role 取解析出的身份键 ----
        virtual std::vector<FeatureDemand> featureDemands() const { return {}; }  // 默认无需求;有状态算子覆盖
        void setFeatureKey(const std::string& role, const std::string& key) { _feature_keys[role] = key; }
        std::string featureKey(const std::string& role) const
        {
            auto it = _feature_keys.find(role);
            return it != _feature_keys.end() ? it->second : role;
        }
        std::map<std::string, std::string> _feature_keys;   // role→袋内身份键(装配期戳入)

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        // 策略的初始化
        virtual void ini(ProblemHandle* problem_handle) {}

        virtual void setProblem(ProblemHandle* problem_handle) {}

        // 在每一代开始前的准备工作(terminator:运行总进度,见 Terminator::getProgress())
        virtual void preparation_s(IndividualArray& population, Terminator* terminator) {}

        // 在每个个体开始前的准备工作
        virtual void preparation_i(Individual* individual, Solution** learning_object, Individual* child) {}

        // 在每一维开始前的准备工作
        virtual void preparation_d(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child) {}

        // 根据学习对象获得下一维的值(默认:该策略不支持逐维构造 → 抛错,由上层记优化日志)
        virtual double nextDecision(const int decision_d, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle, Individual* child)
        {
            throw std::logic_error("Current learning strategy does not support dimensional-by-dimension construction!");
        }

        // 在每一维生成后的更新工作
        virtual void update_d(Individual* child, const int decision_d) {}

        // 在每个个体生成并评估后的更新工作
        virtual void update_i(Individual* child) {}

        // 在子代生成并选择后的更新工作
        virtual void update_s(IndividualArray& population, IndividualArray& offspring, BestArchive* archive) {}

        virtual void getNewIndividual(Individual* child, Individual* individual, Solution** learning_object, ProblemHandle* problem_handle)
        {
            preparation_i(individual, learning_object, child);
            for (int i = 0; i < child->getSolutionSize(); i++)
            {
                preparation_d(i, individual, learning_object, problem_handle, child);
                child->solution.result[i] = nextDecision(i, individual, learning_object, problem_handle, child);
                update_d(child, i);
            }
        }
    };
}
