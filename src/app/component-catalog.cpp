//   方案 A:硬编码列举 12 个 Registry<Base>(见头文件的取舍说明)。
//   **新增组件【类别】时需在本文件的 CATS 表补一行**(新增组件本身无需改动此处)。

// SPDX-License-Identifier: BSD-3-Clause

#include "component-catalog.h"
#include "evoflow.h"
#include "registry.h"
#include "evoflow-assert.h"
#include <cmath>

namespace EvoFlow
{
    namespace App
    {
        namespace
        {
            // ParamInfo → ParamMeta(逐字段直映,不做任何加工)
            ParamMeta toMeta(const ParamInfo& p)
            {
                ParamMeta m;
                m.name = p.name;
                m.kind = (int)p.kind;
                m.low = p.low;   m.high = p.high;
                m.recLow = p.rec_low; m.recHigh = p.rec_high;
                m.allowEmpty = p.allow_empty;
                return m;
            }

            ParamMeta makeMeta(const std::string& name, ParamKind kind,
                               double low, double high, bool allowEmpty,
                               double recLow, double recHigh)
            {
                ParamMeta m;
                m.name = name;
                m.kind = static_cast<int>(kind);
                m.low = low;
                m.high = high;
                m.allowEmpty = allowEmpty;
                m.recLow = recLow;
                m.recHigh = recHigh;
                return m;
            }

            // 从任一 Registry<Base> 收集该类别的全部组件
            template <class Base>
            void collect(ModuleType cat, const std::string& key, std::vector<ComponentMeta>& out)
            {
                const Registry<Base>& R = Registry<Base>::instance();
                std::vector<std::string> ns = R.names();          // names() 早已存在(v2.3.1.2 前无人消费)
                for (size_t i = 0; i < ns.size(); i++)
                {
                    ComponentMeta c;
                    c.tag = ns[i];
                    c.category = cat;
                    c.categoryKey = key;
                    const ParameterTemplate* pt = R.params(ns[i]);
                    if (pt)
                        for (size_t k = 0; k < pt->params.size(); k++)
                            c.params.push_back(toMeta(pt->params[k]));
                    out.push_back(c);
                }
            }

            // 断言列表 → 可读项(经 Assert 的公开访问器 getitem/getNumber/getModuleType;
            //   _item/_number 本身是 private)
            void toItems(AssertList& al, std::vector<RequirementItem>& out)
            {
                for (size_t i = 0; i < al.getSize(); i++)
                {
                    Assert& a = al[(int)i];
                    RequirementItem it;
                    it.item   = a.getitem();
                    it.number = a.getNumber();          // 该项的需求/供给【数量】
                    it.owner  = (int)a.getModuleType(); // 断言归属的组件类别
                    it.detail = it.item + " x" + std::to_string(it.number);
                    out.push_back(it);
                }
            }

            // 取参数取值:不足处用推荐下界补齐(pre/postAssert 会按 paraNum 读取)
            template <class Base>
            std::vector<double> fillPara(const std::string& tag, const std::vector<double>& given)
            {
                std::vector<double> p = given;
                const ParameterTemplate* pt = Registry<Base>::instance().params(tag);
                if (pt)
                    while (p.size() < pt->params.size())
                        p.push_back(pt->params[p.size()].rec_low);
                if (p.empty()) p.push_back(0.0);          // 保证 data() 非空
                return p;
            }

            template <class Base>
            bool queryOne(const std::string& tag, const std::vector<double>& para,
                          bool wantPre, std::vector<RequirementItem>& out)
            {
                const Registry<Base>& R = Registry<Base>::instance();
                if (!R.find(tag)) return false;
                std::vector<double> p = fillPara<Base>(tag, para);
                AssertList al;
                if (wantPre) R.preAssert(tag, al, p.data());
                else         R.postAssert(tag, al, p.data());
                toItems(al, out);
                return true;
            }

            ResolvedParameterSchema resolveTemplate(const ParameterTemplate& root,
                                                    const std::vector<double>& values)
            {
                ResolvedParameterSchema result;
                ParameterTemplate current = root;
                size_t offset = 0;
                const int maxDepth = 8;   // 与 ParamMatcher::MAX_DEPTH 保持同一上限
                for (int depth = 0; depth <= maxDepth; depth++)
                {
                    std::vector<double> level;
                    level.reserve(current.params.size());
                    for (size_t i = 0; i < current.params.size(); i++)
                    {
                        const ParamInfo& info = current.params[i];
                        result.params.push_back(toMeta(info));
                        result.defaults.push_back(info.rec_low);
                        double value = (offset + i < values.size())
                            ? values[offset + i] : info.rec_low;
                        // EMPTYVALUE 无法决定下一级结构;仅用于 schema 披露时采用推荐值,
                        // 原始配置值仍由调用方保留,不会被这里改写。
                        if (std::isnan(value)) value = info.rec_low;
                        level.push_back(value);
                    }
                    offset += current.params.size();

                    if (!current.hasNext())
                    {
                        result.ok = true;
                        return result;
                    }
                    ParameterTemplate next =
                        current.nextLevel(level.empty() ? nullptr : level.data(), level.size());
                    if (next.params.empty() && !next.hasNext())
                    {
                        result.ok = true;
                        return result;
                    }
                    current = next;
                }
                result.error =
                    "parameter disclosure depth exceeds 8 (possible next self-reference)";
                return result;
            }

            template <class Base>
            bool resolveOne(const std::string& tag, const std::vector<double>& values,
                            ResolvedParameterSchema& result)
            {
                const Registry<Base>& registry = Registry<Base>::instance();
                if (!registry.find(tag)) return false;
                const ParameterTemplate* schema = registry.params(tag);
                result = schema
                    ? resolveTemplate(*schema, values)
                    : ResolvedParameterSchema{};
                if (!schema) result.ok = true;
                return true;
            }
        }

        // ================= 12 个类别的清单(方案 A 的唯一维护点) =================
        //   注:categoryKey 覆盖全 12 类。config-codec 的 typeKeyword 只覆盖 workflow 内 6 类
        //   (子群级 6 类在 .cfg 里是独立字段而非 component 行),故此处自成一套且更全。
        struct CatRow { ModuleType type; const char* key; };
        static const CatRow CATS[] = {
            { ModuleType::T_learntopology,      "topology"    },
            { ModuleType::T_learnstrategy,      "strategy"    },
            { ModuleType::T_offspringgenerator, "generator"   },
            { ModuleType::T_selector,           "selector"    },
            { ModuleType::T_evaluator,          "evaluator"   },
            { ModuleType::T_Repair,             "repair"      },
            { ModuleType::T_bestarchive,        "archive"     },
            { ModuleType::T_subswarmmanager,    "manager"     },
            { ModuleType::T_subswarbuilder,     "constructer" },
            { ModuleType::T_subswarmtopology,   "ctopology"   },
            { ModuleType::T_individual,         "individual"  },
            { ModuleType::T_feature,            "feature"     },
        };
        static const int CAT_N = (int)(sizeof(CATS) / sizeof(CATS[0]));

        std::string categoryKeyOf(ModuleType t)
        {
            for (int i = 0; i < CAT_N; i++) if (CATS[i].type == t) return CATS[i].key;
            return "unknown";
        }
        ModuleType categoryOfKey(const std::string& key)
        {
            for (int i = 0; i < CAT_N; i++) if (key == CATS[i].key) return CATS[i].type;
            return ModuleType::T_default;
        }
        std::vector<std::string> categoryKeys()
        {
            std::vector<std::string> v;
            for (int i = 0; i < CAT_N; i++) v.push_back(CATS[i].key);
            return v;
        }

        std::vector<ComponentMeta> listComponents(ModuleType t)
        {
            std::vector<ComponentMeta> out;
            std::string k = categoryKeyOf(t);
            switch (t)
            {
            case ModuleType::T_learntopology:      collect<LearningTopology>(t, k, out); break;
            case ModuleType::T_learnstrategy:      collect<LearningStrategy>(t, k, out); break;
            case ModuleType::T_offspringgenerator: collect<OffspringGenerator>(t, k, out); break;
            case ModuleType::T_selector:           collect<EnvirSelect>(t, k, out); break;
            case ModuleType::T_evaluator:          collect<Evaluator>(t, k, out); break;
            case ModuleType::T_Repair:             collect<SolutionRepaireman>(t, k, out); break;
            case ModuleType::T_bestarchive:        collect<BestArchive>(t, k, out); break;
            case ModuleType::T_subswarmmanager:    collect<SubpopulationManager>(t, k, out); break;
            case ModuleType::T_subswarbuilder:     collect<SubpopulationConstructer>(t, k, out); break;
            case ModuleType::T_subswarmtopology:   collect<SubpopulationTopology>(t, k, out); break;
            case ModuleType::T_individual:         collect<Individual>(t, k, out); break;
            case ModuleType::T_feature:            collect<Feature>(t, k, out); break;
            default: break;
            }
            return out;
        }

        std::vector<ComponentMeta> listComponents()
        {
            std::vector<ComponentMeta> all;
            for (int i = 0; i < CAT_N; i++)
            {
                std::vector<ComponentMeta> one = listComponents(CATS[i].type);
                for (size_t k = 0; k < one.size(); k++) all.push_back(one[k]);
            }
            return all;   // 按 CATS 顺序 + 各表内 map 的字典序 ⇒ 稳定
        }

        std::vector<ComponentMeta> listInitializers()
        {
            std::vector<ComponentMeta> out;
            const char* tags[] = { "Distribution", "Greedy", "No", "R_G", "Random" };
            for (size_t i = 0; i < sizeof(tags) / sizeof(tags[0]); i++)
            {
                ComponentMeta item;
                item.tag = tags[i];
                item.categoryKey = "initializer";
                if (item.tag == "R_G")
                    item.params.push_back(
                        makeMeta("probability", ParamKind::Real, 0.0, 1.0, false, 0.5, 0.5));
                else if (item.tag == "Distribution")
                    item.params.push_back(
                        makeMeta("model", ParamKind::Enum, 1.0, 4.0, false, 1.0, 4.0));
                out.push_back(item);
            }
            return out;
        }

        ResolvedParameterSchema resolveParameterSchema(
            ModuleType t, const std::string& tag, const std::vector<double>& values)
        {
            ResolvedParameterSchema result;
            bool found = false;
            switch (t)
            {
            case ModuleType::T_learntopology:
                found = resolveOne<LearningTopology>(tag, values, result); break;
            case ModuleType::T_learnstrategy:
                found = resolveOne<LearningStrategy>(tag, values, result); break;
            case ModuleType::T_offspringgenerator:
                found = resolveOne<OffspringGenerator>(tag, values, result); break;
            case ModuleType::T_selector:
                found = resolveOne<EnvirSelect>(tag, values, result); break;
            case ModuleType::T_evaluator:
                found = resolveOne<Evaluator>(tag, values, result); break;
            case ModuleType::T_Repair:
                found = resolveOne<SolutionRepaireman>(tag, values, result); break;
            case ModuleType::T_bestarchive:
                found = resolveOne<BestArchive>(tag, values, result); break;
            case ModuleType::T_subswarmmanager:
                found = resolveOne<SubpopulationManager>(tag, values, result); break;
            case ModuleType::T_subswarbuilder:
                found = resolveOne<SubpopulationConstructer>(tag, values, result); break;
            case ModuleType::T_subswarmtopology:
                found = resolveOne<SubpopulationTopology>(tag, values, result); break;
            case ModuleType::T_individual:
                found = resolveOne<Individual>(tag, values, result); break;
            case ModuleType::T_feature:
                found = resolveOne<Feature>(tag, values, result); break;
            default: break;
            }
            if (!found)
                result.error = "unknown " + categoryKeyOf(t) + " tag: '" + tag + "'";
            return result;
        }

        ResolvedParameterSchema resolveParameterSchema(
            const std::string& categoryKey, const std::string& tag,
            const std::vector<double>& values)
        {
            if (categoryKey == "initializer")
            {
                std::vector<ComponentMeta> initializers = listInitializers();
                for (size_t i = 0; i < initializers.size(); i++)
                {
                    if (initializers[i].tag != tag) continue;
                    ResolvedParameterSchema result;
                    result.ok = true;
                    result.params = initializers[i].params;
                    for (size_t k = 0; k < result.params.size(); k++)
                        result.defaults.push_back(result.params[k].recLow);
                    return result;
                }
                ResolvedParameterSchema result;
                result.error = "unknown initializer tag: '" + tag + "'";
                return result;
            }
            const ModuleType type = categoryOfKey(categoryKey);
            if (type == ModuleType::T_default)
            {
                ResolvedParameterSchema result;
                result.error = "unknown component category: '" + categoryKey + "'";
                return result;
            }
            return resolveParameterSchema(type, tag, values);
        }

        std::vector<RequirementItem> queryRequirementsAt(ModuleType t, const std::string& tag,
                                                         const std::vector<double>& para)
        {
            std::vector<RequirementItem> out;
            switch (t)
            {
            case ModuleType::T_learntopology:      queryOne<LearningTopology>(tag, para, true, out); break;
            case ModuleType::T_learnstrategy:      queryOne<LearningStrategy>(tag, para, true, out); break;
            case ModuleType::T_offspringgenerator: queryOne<OffspringGenerator>(tag, para, true, out); break;
            case ModuleType::T_selector:           queryOne<EnvirSelect>(tag, para, true, out); break;
            case ModuleType::T_evaluator:          queryOne<Evaluator>(tag, para, true, out); break;
            case ModuleType::T_Repair:             queryOne<SolutionRepaireman>(tag, para, true, out); break;
            case ModuleType::T_bestarchive:        queryOne<BestArchive>(tag, para, true, out); break;
            case ModuleType::T_subswarmmanager:    queryOne<SubpopulationManager>(tag, para, true, out); break;
            case ModuleType::T_subswarbuilder:     queryOne<SubpopulationConstructer>(tag, para, true, out); break;
            case ModuleType::T_subswarmtopology:   queryOne<SubpopulationTopology>(tag, para, true, out); break;
            case ModuleType::T_individual:         queryOne<Individual>(tag, para, true, out); break;
            case ModuleType::T_feature:            queryOne<Feature>(tag, para, true, out); break;
            default: break;
            }
            return out;
        }

        std::vector<RequirementItem> queryProvisionsAt(ModuleType t, const std::string& tag,
                                                       const std::vector<double>& para)
        {
            std::vector<RequirementItem> out;
            switch (t)
            {
            case ModuleType::T_learntopology:      queryOne<LearningTopology>(tag, para, false, out); break;
            case ModuleType::T_learnstrategy:      queryOne<LearningStrategy>(tag, para, false, out); break;
            case ModuleType::T_offspringgenerator: queryOne<OffspringGenerator>(tag, para, false, out); break;
            case ModuleType::T_selector:           queryOne<EnvirSelect>(tag, para, false, out); break;
            case ModuleType::T_evaluator:          queryOne<Evaluator>(tag, para, false, out); break;
            case ModuleType::T_Repair:             queryOne<SolutionRepaireman>(tag, para, false, out); break;
            case ModuleType::T_bestarchive:        queryOne<BestArchive>(tag, para, false, out); break;
            case ModuleType::T_subswarmmanager:    queryOne<SubpopulationManager>(tag, para, false, out); break;
            case ModuleType::T_subswarbuilder:     queryOne<SubpopulationConstructer>(tag, para, false, out); break;
            case ModuleType::T_subswarmtopology:   queryOne<SubpopulationTopology>(tag, para, false, out); break;
            case ModuleType::T_individual:         queryOne<Individual>(tag, para, false, out); break;
            case ModuleType::T_feature:            queryOne<Feature>(tag, para, false, out); break;
            default: break;
            }
            return out;
        }

        std::vector<RequirementItem> queryRequirementsAtRecommended(ModuleType t, const std::string& tag)
        {
            return queryRequirementsAt(t, tag, std::vector<double>());   // 空 → fillPara 用推荐下界补齐
        }
    }
}
