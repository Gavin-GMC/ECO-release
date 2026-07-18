//------------------------Description------------------------
// 装配期特性推断引擎(INDIV-COMPOSE S2):扫一条 workflow 的组件需求,推断出个体应挂的特性集,
// 并把每个组件"role→袋内身份键"的映射戳回该组件,供运行期按 role 取用。
//-------------------------Reference-------------------------
// 新增。设计见 docs/个体重构方案.md §4-5。
//-----------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <stdexcept>
#include "feature.h"
#include "configure-list.h"
#include "optimize-component.h"

namespace ECFlow
{
    namespace FeatureInference
    {
        // 返回推断特性集(去重、按首见序);副作用:把 role→键 戳回各组件。
        inline std::vector<FeatureSpec> infer(
            const std::vector<ComponentConfig>& configs,
            const std::vector<O_Component*>& comps)
        {
            std::vector<FeatureSpec> ordered;
            std::map<std::string, int> index;   // key → ordered 下标

            int n = (int)comps.size();
            for (int i = 0; i < n; i++)
            {
                std::vector<FeatureDemand> demands = comps[i]->featureDemands();
                if (demands.empty()) continue;

                std::string tag = (i < (int)configs.size()) ? configs[i].tag : std::string();
                std::vector<std::pair<std::string, std::string>> channels;
                if (i < (int)configs.size()) channels = configs[i].feature_channels;

                for (const FeatureDemand& d : demands)
                {
                    // 该 role 是否配了共享通道
                    std::string channel; bool hasChannel = false;
                    for (auto& c : channels) if (c.first == d.role) { channel = c.second; hasChannel = true; break; }

                    // 按 scope 解析身份键
                    std::string key;
                    if (d.scope == FeatureScope::Singular)
                        key = d.role;
                    else if (hasChannel)
                    {
                        if (d.scope == FeatureScope::Exclusive)
                            throw std::runtime_error("[feature] exclusive feature cannot be shared via channel: role '" + d.role + "'");
                        key = channel;
                    }
                    else
                        key = tag + "#" + std::to_string(i) + "." + d.role;

                    // 去重 + kind 一致性 + 初始化策略一致性(共享时多组件声明须一致)
                    auto it = index.find(key);
                    if (it == index.end())
                    {
                        index[key] = (int)ordered.size();
                        ordered.push_back(FeatureSpec{ key, d.kind, d.params, d.init, d.init_value });
                    }
                    else
                    {
                        const FeatureSpec& prev = ordered[it->second];
                        if (prev.kind != d.kind)
                            throw std::runtime_error("[feature] key '" + key + "' kind conflict: '"
                                + prev.kind + "' vs '" + d.kind + "'");
                        if (prev.init != d.init || prev.init_value != d.init_value)
                            throw std::runtime_error("[feature] key '" + key + "' init-policy conflict (shared feature must agree on initialization)");
                    }

                    comps[i]->setFeatureKey(d.role, key);   // 戳回组件
                }
            }
            return ordered;
        }
    }
}
