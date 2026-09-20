//------------------------Description------------------------
// AssertMatcher:装配期 assert 校验。沿 workflow 累积各组件 postAssert 成"已提供上下文",
//   在每个 generator 处对其所用的 topology/strategy/repair(+ generator 自身)统一校 preAssert;
//   个体类型的 postAssert(velocity/pbest…)入上下文。按 (workflow_tag, individual_type) 缓存结果。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "configure-list.h"
#include "evoflow-assert.h"
#include "registry.h"
#include "learning-topologies.h"
#include "learning-strategies.h"
#include "generator.h"
#include "select-basic.h"
#include "select-immune.h"
#include "select-firework.h"
#include "select-rank-crowding.h"
#include "select-scalar-replace.h"
#include "select-kinship.h"
#include "evaluator-basic.h"
#include "evaluator-hash.h"
#include "solution-repairman.h"
#include "individuals.h"

namespace EvoFlow
{
    // 单条诊断使用 Error/Warning/Info；Pass 仅用于“没有任何诊断”的整体结果。
    // 数值按严重度递增，便于聚合时直接取最大值。
    enum class ValidationLevel
    {
        Pass = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };

    inline const char* validationLevelName(ValidationLevel level)
    {
        switch (level)
        {
        case ValidationLevel::Error:   return "error";
        case ValidationLevel::Warning: return "warning";
        case ValidationLevel::Info:    return "info";
        case ValidationLevel::Pass:
        default:                       return "pass";
        }
    }

    // 结构化问题项(v2.3.1.4)。UI 的「优化器构筑页实时校验」要能定位到**是哪个组件、因何触发**,
    //   只给一串消息文本不够 —— 前端得解析字符串才能分类/高亮,那是脆弱的。
    // 【向后兼容】本结构是**加法**:下面的 errors/warnings 原样保留并新增 infos,故既有消费方
    //   (optimizer-builder 拼异常消息、smoke-assert-matcher/smoke-param-matcher 用 .size())零改动。
    struct ValidateIssue
    {
        std::string component;   // 触发组件的 tag(如 "OrderedConstruct"/"VelocityDriven";非组件级问题为空)
        ModuleType  type = ModuleType::T_default;   // 该组件的类别
        int         componentIndex = -1;            // workflow.components 下标;非组件级问题为 -1
        std::string reason;      // **机器可读**的原因键(见下表);UI 据此分类/本地化,不必解析消息文本
        std::string message;     // 完整可读消息(与 errors/warnings/infos 里的那条**逐字相同**)
        ValidationLevel severity = ValidationLevel::Error;
        bool        isError = true;   // 兼容旧消费方；恒等于 severity == Error
    };
    // reason 键(稳定契约,新增需同步手册):
    //   unknown_tag                 组件 tag 未注册
    //   individual_in_workflow      T_individual 出现在 workflow(应在子群级)
    //   missing_provision           所需项无人提供
    //   insufficient_number         提供方数量不足
    //   imprecise_match             满足但不精确(Notice=Info,Warning=Warning)
    //   advisory_not_provided       非强制依赖未提供(Notice=Info,Warning=Warning)
    //   no_topology_before_generator / no_strategy_before_generator / no_repair_before_generator
    //   no_generator_since_selector 选择器前无生成器(无可选之物)
    //   inplace_requires_pending_offspring   原地生成前无未评估、未合入的子代
    //   inplace_requires_offspring_topology  原地生成所用拓扑的起点不来自当前子代
    //   wave_refraction_without_height_progress  折射激活/重置存在但整条 workflow 无波高推进段(警告)
    //   wave_refraction_selector_semantics       折射后未使用无条件按索引替换(信息)
    //   param_count_mismatch        参数元数与模板不符
    //   param_out_of_range          参数越界
    //   param_not_integer           声明 Int/Enum 却给了非整值
    //   param_empty_not_allowed     给了 EMPTYVALUE 但模板不允许

    struct ValidateResult
    {
        bool                     valid = true;
        std::vector<std::string> errors;    // 硬失败
        std::vector<std::string> warnings;  // 可构建但无法保证正常执行/退出
        std::vector<std::string> infos;     // 可正常执行但存在语义偏差或无害冗余
        std::vector<ValidateIssue> issues;  // 与 errors+warnings+infos 一一对应

        // 内部:同时填两侧,保证 issues 与 errors/warnings/infos 不脱节
        void addError(const std::string& msg, const std::string& comp,
                      ModuleType t, const std::string& why, int componentIndex = -1)
        {
            errors.push_back(msg);
            ValidateIssue i; i.component = comp; i.type = t; i.componentIndex = componentIndex;
            i.reason = why; i.message = msg; i.severity = ValidationLevel::Error; i.isError = true;
            issues.push_back(i);
            valid = false;
        }
        void addWarning(const std::string& msg, const std::string& comp,
                        ModuleType t, const std::string& why, int componentIndex = -1)
        {
            warnings.push_back(msg);
            ValidateIssue i; i.component = comp; i.type = t; i.componentIndex = componentIndex;
            i.reason = why; i.message = msg; i.severity = ValidationLevel::Warning; i.isError = false;
            issues.push_back(i);
        }

        void addInfo(const std::string& msg, const std::string& comp,
                     ModuleType t, const std::string& why, int componentIndex = -1)
        {
            infos.push_back(msg);
            ValidateIssue i; i.component = comp; i.type = t; i.componentIndex = componentIndex;
            i.reason = why; i.message = msg; i.severity = ValidationLevel::Info; i.isError = false;
            issues.push_back(i);
        }

        ValidationLevel level() const
        {
            if (!errors.empty()) return ValidationLevel::Error;
            if (!warnings.empty()) return ValidationLevel::Warning;
            if (!infos.empty()) return ValidationLevel::Info;
            return ValidationLevel::Pass;
        }
    };

    class AssertMatcher
    {
    private:
        // preAssert 方法直接读 paras[k],补零缓冲防越界
        struct ParaBuf { double p[32]; ParaBuf(const std::vector<double>& v) { for (int i = 0; i < 32; i++) p[i] = 0; for (size_t i = 0; i < v.size() && i < 32; i++) p[i] = v[i]; } };

        // postAssert 在注册表中由 AssertList(拥有裸指针)承载，不适合复制/替换。
        // Matcher 内部转成值语义快照，使“当前组件能力”和“运行状态”可以独立更新，
        // 不再把历次 SET 过的组件能力永久堆进一个 append-only ctx。
        struct Provision
        {
            ModuleType type = ModuleType::T_default;
            std::string item;
            int number = 0;
        };
        using ProvisionList = std::vector<Provision>;

        struct ProviderContexts
        {
            const ProvisionList* topology = nullptr;       // 当前有效 learning graph 的结构能力
            const ProvisionList* strategy = nullptr;       // 当前 cur_strat 自身能力
            const ProvisionList* repair = nullptr;         // 当前 cur_repair 自身能力
            const ProvisionList* generatorState = nullptr; // 当前 offspring/last_graph 运行状态
            const ProvisionList* selectorWorkflow = nullptr;// workflow 级 selector 能力(当前仅 ageMaintained)
            const ProvisionList* evaluator = nullptr;

            const ProvisionList& forType(ModuleType t) const
            {
                static const ProvisionList empty;
                switch (t)
                {
                case ModuleType::T_learntopology:      return topology ? *topology : empty;
                case ModuleType::T_learnstrategy:      return strategy ? *strategy : empty;
                case ModuleType::T_Repair:             return repair ? *repair : empty;
                case ModuleType::T_offspringgenerator: return generatorState ? *generatorState : empty;
                case ModuleType::T_selector:           return selectorWorkflow ? *selectorWorkflow : empty;
                case ModuleType::T_evaluator:          return evaluator ? *evaluator : empty;
                default:                               return empty;
                }
            }
        };

        static std::string mtName(ModuleType t)
        {
            switch (t)
            {
            case ModuleType::T_individual:         return "individual";
            case ModuleType::T_learntopology:      return "topology";
            case ModuleType::T_learnstrategy:      return "strategy";
            case ModuleType::T_offspringgenerator: return "generator";
            case ModuleType::T_selector:           return "selector";
            case ModuleType::T_evaluator:          return "evaluator";
            case ModuleType::T_Repair:             return "repair";
            default:                               return "?";
            }
        }

        static bool isRegistered(ModuleType t, const std::string& tag)
        {
            switch (t)
            {
            case ModuleType::T_learntopology:      return Registry<LearningTopology>::instance().find(tag) != nullptr;
            case ModuleType::T_learnstrategy:      return Registry<LearningStrategy>::instance().find(tag) != nullptr;
            case ModuleType::T_offspringgenerator: return Registry<OffspringGenerator>::instance().find(tag) != nullptr;
            case ModuleType::T_selector:           return Registry<EnvirSelect>::instance().find(tag) != nullptr;
            case ModuleType::T_evaluator:          return Registry<Evaluator>::instance().find(tag) != nullptr;
            case ModuleType::T_Repair:             return Registry<SolutionRepaireman>::instance().find(tag) != nullptr;
            // T_individual:个体只剩基类(Registry<Individual> 已移除)→ 无"类型"可查,恒有效。
            //   v1.4.8 已删 N6 个体上下文校验,故本分支现无调用方;**枚举值保留**(Assert::module_type 仍在用),
            //   本分支一并保留以防将来有个体级断言复活。
            case ModuleType::T_individual:         return true;
            default:                               return false;
            }
        }
        static void fillPre(ModuleType t, const std::string& tag, const double* p, AssertList& out)
        {
            switch (t)
            {
            case ModuleType::T_learntopology:      Registry<LearningTopology>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_learnstrategy:      Registry<LearningStrategy>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_offspringgenerator: Registry<OffspringGenerator>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_selector:           Registry<EnvirSelect>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_evaluator:          Registry<Evaluator>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_Repair:             Registry<SolutionRepaireman>::instance().preAssert(tag, out, p); break;
            case ModuleType::T_individual:         break;   // 基类个体无 preAssert(需求全在组件 featureDemands)
            default: break;
            }
        }
        static void fillPost(ModuleType t, const std::string& tag, const double* p, AssertList& out)
        {
            switch (t)
            {
            case ModuleType::T_learntopology:      Registry<LearningTopology>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_learnstrategy:      Registry<LearningStrategy>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_offspringgenerator: Registry<OffspringGenerator>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_selector:           Registry<EnvirSelect>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_evaluator:          Registry<Evaluator>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_Repair:             Registry<SolutionRepaireman>::instance().postAssert(tag, out, p); break;
            case ModuleType::T_individual:         break;   // 基类个体不提供上下文(velocity/pbest 等由特性推断,非个体声明)
            default: break;
            }
        }

        static ProvisionList collectPost(ModuleType t, const std::string& tag, const std::vector<double>& para)
        {
            ParaBuf pb(para);
            AssertList post;
            fillPost(t, tag, pb.p, post);
            ProvisionList out;
            out.reserve(post.getSize());
            for (int i = 0; i < (int)post.getSize(); i++)
            {
                Assert& a = post[i];
                Provision p; p.type = a.getModuleType(); p.item = a.getitem(); p.number = a.getNumber();
                out.push_back(p);
            }
            return out;
        }

        static bool hasProvision(const ProvisionList& list, ModuleType type, const std::string& item, int minimum = 1)
        {
            for (const Provision& p : list)
                if (p.type == type && p.item == item && p.number >= minimum) return true;
            return false;
        }

        static void appendUnique(ProvisionList& dst, const Provision& value)
        {
            for (Provision& p : dst)
                if (p.type == value.type && p.item == value.item)
                { p.number = value.number; return; }
            dst.push_back(value);
        }

        // NoChange 复制 last_graph，只有图的结构属性可以继承；起点来源、激活行为等
        // 必须由当前 topology 自身明确声明，不能随历史能力传播。
        static bool isGraphStructureItem(const Provision& p)
        {
            return p.type == ModuleType::T_learntopology &&
                   (p.item == "objects" || p.item == "coupled" || p.item == "graphScale");
        }

        static ProvisionList effectiveTopologyPost(const ComponentConfig& topology,
                                                   const ProvisionList& lastGraphStructure)
        {
            ProvisionList current = collectPost(topology.c_type, topology.tag, topology.para);
            if (!hasProvision(current, ModuleType::T_learntopology, "reuseGraphStructure")) return current;
            for (const Provision& p : lastGraphStructure)
                if (isGraphStructureItem(p)) appendUnique(current, p);
            return current;
        }

        static ProvisionList graphStructureOf(const ProvisionList& topology)
        {
            ProvisionList out;
            for (const Provision& p : topology)
                if (isGraphStructureItem(p)) appendUnique(out, p);
            return out;
        }

        // 单条 preAssert 只对其语义所属的**当前上下文**匹配。
        // current topology/strategy/repair 能力不再从历史组件继承；跨段条件由 generatorState
        // 或 selectorWorkflow 的显式上下文承载。
        // v2.3.1.4:多带 srcTag/srcType —— 结构化问题项要记「是哪个组件触发的」。
        //   who 是给人读的描述串(如 "strategy 'VelocityDriven'"),srcTag 是机器用的裸 tag。
        static void checkOne(Assert& req, const ProvisionList& ctx, const std::string& who, ValidateResult& r,
                             const std::string& srcTag = "", ModuleType srcType = ModuleType::T_default,
                             int srcIndex = -1)
        {
            for (const Provision& prov : ctx)
            {
                if (prov.type != req.getModuleType() || prov.item != req.getitem()) continue;

                int a = prov.number, e = req.getNumber();
                bool fail = false, softMismatch = false;
                switch (req.getMatchType())
                {
                case MatchType::notLess:          fail = (a < e); break;
                case MatchType::notLessButNotice: fail = (a < e); softMismatch = (a > e); break;
                case MatchType::equal:            fail = (a != e); break;
                case MatchType::anyButNotice:     softMismatch = (a != e); break;
                case MatchType::anyButWarning:    softMismatch = (a != e); break;
                case MatchType::postAssert:       break;
                }
                if (fail)
                {
                    r.addError(who + ": needs " + mtName(req.getModuleType()) + "." + req.getitem() +
                        " (expect " + std::to_string(e) + ", provided " + std::to_string(a) + ")",
                        srcTag, srcType, "insufficient_number", srcIndex);
                    r.valid = false;
                }
                else if (softMismatch)
                {
                    const std::string msg = who + ": " + mtName(req.getModuleType()) + "." + req.getitem() +
                        " provided " + std::to_string(a) + " vs expect " + std::to_string(e) + " (imprecise)";
                    // MatchResult 只描述“非精确匹配”；严重级由 MatchType 显式决定。
                    if (req.getMatchType() == MatchType::anyButWarning)
                        r.addWarning(msg, srcTag, srcType, "imprecise_match", srcIndex);
                    else
                        r.addInfo(msg, srcTag, srcType, "imprecise_match", srcIndex);
                }
                return;   // 找到该项即止
            }
            // 未找到:Notice=Info,Warning=Warning;其余(notLess/equal/notLessButNotice)→ 硬失败。
            if (req.getMatchType() == MatchType::anyButNotice)
                r.addInfo(who + ": " + mtName(req.getModuleType()) + "." + req.getitem() + " not provided (advisory)",
                          srcTag, srcType, "advisory_not_provided", srcIndex);
            else if (req.getMatchType() == MatchType::anyButWarning)
                r.addWarning(who + ": " + mtName(req.getModuleType()) + "." + req.getitem() + " not provided (advisory)",
                             srcTag, srcType, "advisory_not_provided", srcIndex);
            else
            {
                r.addError(who + ": requires " + mtName(req.getModuleType()) + "." + req.getitem() + " but none provided",
                           srcTag, srcType, "missing_provision", srcIndex);
                r.valid = false;
            }
        }

        // 校一个组件的全部 preAssert
        static void checkComponent(ModuleType t, const std::string& tag, const std::vector<double>& para,
                                   const ProviderContexts& contexts, const std::string& who, ValidateResult& r,
                                   int componentIndex)
        {
            ParaBuf pb(para);
            AssertList pre;
            fillPre(t, tag, pb.p, pre);
            for (int i = 0; i < pre.getSize(); i++)
                checkOne(pre[i], contexts.forType(pre[i].getModuleType()), who, r, tag, t, componentIndex);
        }

        // 查询当前组件自己的 postAssert 能力。不能直接查累计 ctx:较早设置的同类组件标记
        // 仍留在 ctx 中,会让后续不具备该能力的组件误继承旧标记。
        static bool hasPostCapability(ModuleType t, const std::string& tag, const std::vector<double>& para,
                                      ModuleType capabilityType, const std::string& item, int minimum = 1)
        {
            ParaBuf pb(para);
            AssertList post;
            fillPost(t, tag, pb.p, post);
            for (int i = 0; i < post.getSize(); i++)
            {
                Assert& a = post[i];
                if (a.getModuleType() == capabilityType && a.getitem() == item && a.getNumber() >= minimum)
                    return true;
            }
            return false;
        }

    public:
        // 校验一条 workflow。按 workflow_tag 缓存,一 workflow 多子群只校一次。
        //   v1.4.8:**删去 individual_type 形参与 N6 个体上下文**(ASSERT-N6-CLEANUP 落地)——
        //   INDIV-COMPOSE 收官后个体只剩基类、状态需求改由组件 featureDemands() 声明 + 装配期推断供给,
        //   旧的"个体类型提供 velocity/pbest 上下文"机制**无人产出**:isRegistered(T_individual,…) 早已恒真
        //   (Registry<Individual> 已移除)→ 那条 unknown individual type 错误**永不触发**、addPost 也只加了个空。
        //   缓存键随之由 (workflow_tag, individual_type) 退化为 workflow_tag。
        //   **保留** N1–N5 组件间校验 + constructive/postAssert 机制。
        // 【v2.3.1.4 移除按 wf.tag 的结果缓存】原实现:`static map<string, ValidateResult> cache`
        //   以 **wf.tag 为键**。但 **tag 完全不能标识 workflow 的内容** —— 两份组件列表迥异的
        //   workflow 只要同名,第二份就直接套用第一份的结论。
        //   实测:`config/` 下 **97 个配置的 workflow tag 全是 `wf`**(一个不差)⇒ 同一进程内批量
        //   校验时,**只有第一个被真正验过,其余 96 个全是缓存命中**;坏配置也会拿到好配置的结论。
        //   装配路径下这个缺陷不易暴露(一次进程通常只装配一套配置),但 `evoflow validate` 的
        //   全部意义就是在同一进程里批量验多个文件 —— 缓存在此直接致命。
        //   缓存原为"同一 workflow 被多个子群引用"而设,那种场景下 tag 相同**且内容确实相同**;
        //   一旦离开该假设即错。校验本身开销很小,故直接去掉而非改键。
        static ValidateResult validate(const WorkflowConfig& wf)
        {
            ValidateResult r;

            // workflow 级能力与执行顺序无关。当前只有 ageMaintained：Aging selector
            // 负责挂载并维护 Singular age，AgeActivation 可在任一段消费它。
            ProvisionList workflow_selector_ctx;
            for (const ComponentConfig& c : wf.components)
            {
                if (c.c_type != ModuleType::T_selector || !isRegistered(c.c_type, c.tag)) continue;
                ProvisionList post = collectPost(c.c_type, c.tag, c.para);
                for (const Provision& p : post)
                    if (p.type == ModuleType::T_selector && p.item == "ageMaintained")
                        appendUnique(workflow_selector_ctx, p);
            }

            const ComponentConfig* cur_topo = nullptr;
            const ComponentConfig* cur_strat = nullptr;
            const ComponentConfig* cur_repair = nullptr;
            int cur_topo_index = -1, cur_strat_index = -1, cur_repair_index = -1;
            bool cur_topo_starts_from_offspring = false;

            ProvisionList active_topology_ctx;
            ProvisionList active_strategy_ctx;
            ProvisionList active_repair_ctx;
            ProvisionList generator_state_ctx;
            ProvisionList last_graph_structure;

            // N3/N4 状态与计数
            int  n_gen = 0, n_sel = 0;
            bool gen_since_sel = false;   // 上次 selector 之后是否出现过 generator
            bool eval_since_gen = false;  // 上次 generator 之后是否出现过 evaluator
            bool offspring_available = false; // 当前 offspring buffer 中存在可供续接拓扑使用的子代
            bool pending_unevaluated_offspring = false; // 可供 inplace generator 继续修改的当前子代
            bool last_graph_available = false; // 本次 workflow.run 内已有 generator 产出 last_learning_graph
            auto refreshGeneratorState = [&]() {
                generator_state_ctx.clear();
                if (offspring_available)
                    generator_state_ctx.push_back({ ModuleType::T_offspringgenerator, "offspringAvailable", 1 });
                if (pending_unevaluated_offspring)
                    generator_state_ctx.push_back({ ModuleType::T_offspringgenerator, "unevaluatedOffspringAvailable", 1 });
                if (last_graph_available)
                    generator_state_ctx.push_back({ ModuleType::T_offspringgenerator, "lastGraphAvailable", 1 });
            };
            bool workflow_progresses_wave_height = false;
            bool pending_refraction_pairing = false;
            struct RefractionRiskSource { std::string tag; ModuleType type; int index; };
            std::vector<RefractionRiskSource> refraction_risk_sources;

            for (size_t componentIndex = 0; componentIndex < wf.components.size(); componentIndex++)
            {
                const ComponentConfig& c = wf.components[componentIndex];
                const int ci = static_cast<int>(componentIndex);
                if (c.c_type == ModuleType::T_individual)
                { r.addError("T_individual must not appear in workflow (set at subpopulation level)", c.tag, c.c_type, "individual_in_workflow", ci); r.valid = false; continue; }

                // N5:tag 合法
                if (!isRegistered(c.c_type, c.tag))
                { r.addError("unknown " + mtName(c.c_type) + " tag: '" + c.tag + "'", c.tag, c.c_type, "unknown_tag", ci); r.valid = false; continue; }

                switch (c.c_type)
                {
                case ModuleType::T_learntopology:
                    cur_topo = &c;
                    cur_topo_index = ci;
                    cur_topo_starts_from_offspring = hasPostCapability(
                        c.c_type, c.tag, c.para, ModuleType::T_learntopology, "offspringStart");
                    break;
                case ModuleType::T_learnstrategy: cur_strat = &c; cur_strat_index = ci; break;
                case ModuleType::T_Repair:        cur_repair = &c; cur_repair_index = ci; break;
                case ModuleType::T_evaluator:
                    eval_since_gen = true;
                    pending_unevaluated_offspring = false;
                    refreshGeneratorState();
                    break;

                case ModuleType::T_offspringgenerator:
                {
                    std::string g = "generator '" + c.tag + "'";
                    const bool inplace = !c.para.empty() && c.para[0] != 0.0;
                    // N2:前置组件须已 SET(吸收 REPAIR-REQUIRED)
                    if (!cur_topo)  { r.addError(g + ": no topology set before it", c.tag, c.c_type, "no_topology_before_generator", ci); r.valid = false; }
                    if (!cur_strat) { r.addError(g + ": no strategy set before it", c.tag, c.c_type, "no_strategy_before_generator", ci); r.valid = false; }
                    if (!cur_repair){ r.addError(g + ": no repair set before it", c.tag, c.c_type, "no_repair_before_generator", ci); r.valid = false; }
                    // inplace 不创建子代,只继续修改当前 offspring。它要求子代仍未评估/未合入,
                    // 且当前 topology 的起点明确来自 offspring,否则运行时会空转或跳过已评估亲代。
                    if (inplace && !pending_unevaluated_offspring)
                    {
                        r.addError(g + ": inplace mode requires pending unevaluated offspring before evaluation or selection",
                                   c.tag, c.c_type, "inplace_requires_pending_offspring", ci);
                        r.valid = false;
                    }
                    if (inplace && !cur_topo_starts_from_offspring)
                    {
                        r.addError(g + ": inplace mode requires a topology whose start points come from current offspring",
                                   c.tag, c.c_type, "inplace_requires_offspring_topology", ci);
                        r.valid = false;
                    }

                    // 当前能力快照：每个 SET 槽都只读取当前组件。NoChange 的结构能力来自
                    // 当前 last_graph 快照；其它拓扑由自身 postAssert 完整描述。
                    active_topology_ctx = cur_topo ? effectiveTopologyPost(*cur_topo, last_graph_structure) : ProvisionList{};
                    active_strategy_ctx = cur_strat ? collectPost(cur_strat->c_type, cur_strat->tag, cur_strat->para) : ProvisionList{};
                    active_repair_ctx = cur_repair ? collectPost(cur_repair->c_type, cur_repair->tag, cur_repair->para) : ProvisionList{};
                    refreshGeneratorState();

                    ProviderContexts contexts;
                    contexts.topology = &active_topology_ctx;
                    contexts.strategy = &active_strategy_ctx;
                    contexts.repair = &active_repair_ctx;
                    contexts.generatorState = &generator_state_ctx;
                    contexts.selectorWorkflow = &workflow_selector_ctx;

                    // N1:generator 处统一校 所用 topology/strategy/repair + generator 自身 的 preAssert。
                    // 所有能力均来自上面的当前快照，跨段运行条件只来自 generator_state_ctx。
                    if (cur_topo)  checkComponent(ModuleType::T_learntopology, cur_topo->tag, cur_topo->para, contexts, "topology '" + cur_topo->tag + "'", r, cur_topo_index);
                    if (cur_strat) checkComponent(ModuleType::T_learnstrategy, cur_strat->tag, cur_strat->para, contexts, "strategy '" + cur_strat->tag + "'", r, cur_strat_index);
                    if (cur_repair)checkComponent(ModuleType::T_Repair, cur_repair->tag, cur_repair->para, contexts, "repair '" + cur_repair->tag + "'", r, cur_repair_index);
                    checkComponent(ModuleType::T_offspringgenerator, c.tag, c.para, contexts, g, r, ci);

                    // WWO 状态契约：WaveRefractionActivation 会按 h 激活，WaveRefraction 又把 h 重置为正值；
                    // 整条循环若没有任何真正执行的波高推进段，种群可进入“全直通、零评估”的吸收态。
                    const bool progressesWaveHeight = cur_strat && hasPostCapability(
                        cur_strat->c_type, cur_strat->tag, cur_strat->para,
                        ModuleType::T_learnstrategy, "waveHeightProgress");
                    const bool activatesByWaveHeight = cur_topo && hasPostCapability(
                        cur_topo->c_type, cur_topo->tag, cur_topo->para,
                        ModuleType::T_learntopology, "waveHeightActivation");
                    const bool resetsWaveHeight = cur_strat && hasPostCapability(
                        cur_strat->c_type, cur_strat->tag, cur_strat->para,
                        ModuleType::T_learnstrategy, "waveHeightReset");
                    const bool needsRefractionPairing = cur_strat && hasPostCapability(
                        cur_strat->c_type, cur_strat->tag, cur_strat->para,
                        ModuleType::T_learnstrategy, "requiresUnconditionalIndex");
                    if (progressesWaveHeight) workflow_progresses_wave_height = true;
                    if (activatesByWaveHeight && resetsWaveHeight)
                    {
                        RefractionRiskSource src = { cur_strat->tag, cur_strat->c_type, cur_strat_index };
                        refraction_risk_sources.push_back(src);
                    }
                    if (needsRefractionPairing) pending_refraction_pairing = true;

                    // N3 状态:本次 generator 后,gen 已现、eval 归零
                    n_gen++; gen_since_sel = true; eval_since_gen = false;
                    offspring_available = true;
                    if (!inplace) pending_unevaluated_offspring = true;
                    last_graph_available = true;
                    last_graph_structure = graphStructureOf(active_topology_ctx);
                    refreshGeneratorState();
                    break;
                }
                case ModuleType::T_selector:
                {
                    std::string s = "selector '" + c.tag + "'";
                    // N3:选择前须有(上次选择后的)generator,且该 generator 后须有 evaluator
                    if (!gen_since_sel)
                    { r.addError(s + ": no generator since last selector (nothing to select)", c.tag, c.c_type, "no_generator_since_selector", ci); r.valid = false; }
                    else if (!eval_since_gen)
                    { r.addError(s + ": no evaluator after last generator (unevaluated offspring)", c.tag, c.c_type, "no_evaluator_after_generator", ci); r.valid = false; }
                    ProviderContexts contexts;
                    contexts.topology = &active_topology_ctx;
                    contexts.strategy = &active_strategy_ctx;
                    contexts.repair = &active_repair_ctx;
                    contexts.generatorState = &generator_state_ctx;
                    contexts.selectorWorkflow = &workflow_selector_ctx;
                    checkComponent(ModuleType::T_selector, c.tag, c.para, contexts, s, r, ci);
                    if (pending_refraction_pairing)
                    {
                        const bool compatible = hasPostCapability(
                            c.c_type, c.tag, c.para, ModuleType::T_selector,
                            "unconditionalIndexReplacement");
                        if (!compatible)
                            r.addInfo(s + ": WaveRefraction expects unconditional index-aligned replacement; " +
                                      "this selector may change wavelength-update and pass-through semantics",
                                      c.tag, c.c_type, "wave_refraction_selector_semantics", ci);
                    }
                    // 重置
                    n_sel++; gen_since_sel = false; eval_since_gen = false;
                    offspring_available = false;
                    pending_unevaluated_offspring = false;
                    pending_refraction_pairing = false;
                    refreshGeneratorState();
                    break;
                }

                default: break;
                }
            }

            // N4:结构完整性(≥1 generator 防死循环退不出;≥1 selector 子代无从并入亲代)
            if (n_gen < 1) { r.addError("workflow has no generator (offspring never produced -> cannot terminate by FES)", "", ModuleType::T_default, "workflow_no_generator"); r.valid = false; }
            if (n_sel < 1) { r.addError("workflow has no selector (offspring never merged into parent)", "", ModuleType::T_default, "workflow_no_selector"); r.valid = false; }
            if (!workflow_progresses_wave_height)
            {
                for (const RefractionRiskSource& src : refraction_risk_sources)
                    r.addWarning("strategy '" + src.tag + "': wave-height refraction is present but no generator stage " +
                                 "maintains/decrements wave height; the workflow may enter a zero-evaluation loop",
                                 src.tag, src.type, "wave_refraction_without_height_progress", src.index);
            }

            return r;
        }
    };
}
