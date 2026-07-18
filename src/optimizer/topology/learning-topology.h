//------------------------Description------------------------
// 学习拓扑 LearningTopology:决定"谁向谁学习"——为种群构建学习图(LearningGraph)。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include "individual-array.h"
#include "best-archive.h"
#include <map>
#include <string>
#include <vector>
#include "learning-graph.h"
#include "parameter-template.h"
#include "feature.h"

namespace ECFlow
{
    class LearningTopology
    {
    public:
        LearningTopology() {}

        virtual ~LearningTopology() {}

        // ---- INDIV-COMPOSE 特性感知(S2) ----
        virtual std::vector<FeatureDemand> featureDemands() const { return {}; }
        void setFeatureKey(const std::string& role, const std::string& key) { _feature_keys[role] = key; }
        std::string featureKey(const std::string& role) const
        {
            auto it = _feature_keys.find(role);
            return it != _feature_keys.end() ? it->second : role;
        }
        std::map<std::string, std::string> _feature_keys;   // role→袋内身份键(装配期戳入)

        static ParameterTemplate getParameterTemplate() { return ParameterTemplate{}; }

        virtual void ini() = 0;

        virtual LearningGraph* getTopology(IndividualArray** subswarm, BestArchive** best_holder,
                                           const int swarm_number, IndividualArray* offspring, LearningGraph* last_graph) = 0;
    };
}
