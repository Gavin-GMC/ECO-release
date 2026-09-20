//------------------------Description------------------------
// 学习拓扑 LearningTopology:决定"谁向谁学习"——为种群构建学习图(LearningGraph)。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include "individual-array.h"
#include "best-archive.h"
#include <map>
#include <string>
#include <vector>
#include "learning-graph.h"
#include "parameter-template.h"
#include "feature.h"

namespace EvoFlow
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
