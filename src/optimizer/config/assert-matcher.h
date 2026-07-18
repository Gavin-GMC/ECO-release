//------------------------Description------------------------
// AssertMatcher:装配期 assert 校验。沿 workflow 累积各组件 postAssert 成"已提供上下文",
//   在每个 generator 处对其所用的 topology/strategy/repair(+ generator 自身)统一校 preAssert;
//   个体类型的 postAssert(velocity/pbest…)入上下文。按 (workflow_tag, individual_type) 缓存结果。
//-------------------------Reference-------------------------
// 全新实现(原 assert-matcher.h 基于 enum/工厂/旧 ConfigureList,且 prov.match(req) 依赖 postAssert 的
//   nullptr match_func 会崩,故重写);复用 assert 原语 + 注册表已接的算子 pre/postAssert。
//-------------------------Copyright-------------------------
// Copyright (c) 2024 所有人名称（待确认）, All Rights Reserved.
// You are free to use the ECFlow（待确认） for research purposes.
// All publications which use this library or any code in the library
// should acknowledge the use of "ECFlow" and reference
// "未确定"
//-----------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "configure-list.h"
#include "ecflow-assert.h"
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

namespace ECFlow
{
    struct ValidateResult
    {
        bool                     valid = true;
        std::vector<std::string> errors;    // 硬失败
        std::vector<std::string> warnings;  // 不精确等提示
    };

    class AssertMatcher
    {
    private:
        // preAssert 方法直接读 paras[k],补零缓冲防越界
        struct ParaBuf { double p[32]; ParaBuf(const std::vector<double>& v) { for (int i = 0; i < 32; i++) p[i] = 0; for (size_t i = 0; i < v.size() && i < 32; i++) p[i] = v[i]; } };

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

        // 单条 preAssert 对上下文匹配(同 module_type + item 才比;按 MatchType 语义比 number)
        //   **倒序**遍历 ctx:取**最近**的提供者。多段 workflow 下 `objects`/`graphScale` 等是"每段拓扑各自的属性",
        //   该段策略的供给方是**本段**的拓扑(最近入 ctx 者);正序会被**前面段**的同名项遮蔽而误判
        //   (如:段① isolate objects=0 → 段② 需 objects≥1 的策略被误判失败)。单一提供者时正倒序等价。
        static void checkOne(Assert& req, AssertList& ctx, const std::string& who, ValidateResult& r)
        {
            for (int j = ctx.getSize() - 1; j >= 0; j--)
            {
                Assert& prov = ctx[j];
                if (prov.getModuleType() != req.getModuleType() || prov.getitem() != req.getitem()) continue;

                int a = prov.getNumber(), e = req.getNumber();
                bool fail = false, notice = false;
                switch (req.getMatchType())
                {
                case MatchType::notLess:          fail = (a < e); break;
                case MatchType::notLessButNotice: fail = (a < e); notice = (a > e); break;
                case MatchType::equal:            fail = (a != e); break;
                case MatchType::anyButNotice:     notice = (a != e); break;
                case MatchType::postAssert:       break;
                }
                if (fail)
                {
                    r.errors.push_back(who + ": needs " + mtName(req.getModuleType()) + "." + req.getitem() +
                        " (expect " + std::to_string(e) + ", provided " + std::to_string(a) + ")");
                    r.valid = false;
                }
                else if (notice)
                {
                    r.warnings.push_back(who + ": " + mtName(req.getModuleType()) + "." + req.getitem() +
                        " provided " + std::to_string(a) + " vs expect " + std::to_string(e) + " (imprecise)");
                }
                return;   // 找到该项即止
            }
            // 未找到:anyButNotice 为纯建议级 → 仅警告;其余(notLess/equal/notLessButNotice)→ 硬失败
            if (req.getMatchType() == MatchType::anyButNotice)
                r.warnings.push_back(who + ": " + mtName(req.getModuleType()) + "." + req.getitem() + " not provided (advisory)");
            else
            {
                r.errors.push_back(who + ": requires " + mtName(req.getModuleType()) + "." + req.getitem() + " but none provided");
                r.valid = false;
            }
        }

        // 校一个组件的全部 preAssert
        static void checkComponent(ModuleType t, const std::string& tag, const std::vector<double>& para, AssertList& ctx, const std::string& who, ValidateResult& r)
        {
            ParaBuf pb(para);
            AssertList pre;
            fillPre(t, tag, pb.p, pre);
            for (int i = 0; i < pre.getSize(); i++) checkOne(pre[i], ctx, who, r);
        }
        static void addPost(ModuleType t, const std::string& tag, const std::vector<double>& para, AssertList& ctx)
        {
            ParaBuf pb(para); fillPost(t, tag, pb.p, ctx);
        }

    public:
        // 校验一条 workflow。按 workflow_tag 缓存,一 workflow 多子群只校一次。
        //   INDIV-COMPOSE 收官后个体只剩基类、状态需求改由组件 featureDemands() 声明 + 装配期推断供给,
        //   旧的"个体类型提供 velocity/pbest 上下文"机制**无人产出**:isRegistered(T_individual,…) 早已恒真
        //   (Registry<Individual> 已移除)→ 那条 unknown individual type 错误**永不触发**、addPost 也只加了个空。
        //   缓存键随之由 (workflow_tag, individual_type) 退化为 workflow_tag。
        //   **保留** N1–N5 组件间校验 + constructive/postAssert 机制。
        static ValidateResult validate(const WorkflowConfig& wf)
        {
            static std::map<std::string, ValidateResult> cache;
            auto it = cache.find(wf.tag);
            if (it != cache.end()) return it->second;

            ValidateResult r;
            AssertList ctx;   // 已提供上下文(拥有 Assert*,函数末析构)

            const ComponentConfig* cur_topo = nullptr;
            const ComponentConfig* cur_strat = nullptr;
            const ComponentConfig* cur_repair = nullptr;

            // N3/N4 状态与计数
            int  n_gen = 0, n_sel = 0;
            bool gen_since_sel = false;   // 上次 selector 之后是否出现过 generator
            bool eval_since_gen = false;  // 上次 generator 之后是否出现过 evaluator

            for (const ComponentConfig& c : wf.components)
            {
                if (c.c_type == ModuleType::T_individual)
                { r.errors.push_back("T_individual must not appear in workflow (set at subpopulation level)"); r.valid = false; continue; }

                // N5:tag 合法
                if (!isRegistered(c.c_type, c.tag))
                { r.errors.push_back("unknown " + mtName(c.c_type) + " tag: '" + c.tag + "'"); r.valid = false; continue; }

                switch (c.c_type)
                {
                case ModuleType::T_learntopology: cur_topo = &c;  addPost(c.c_type, c.tag, c.para, ctx); break;
                case ModuleType::T_learnstrategy: cur_strat = &c; addPost(c.c_type, c.tag, c.para, ctx); break;
                case ModuleType::T_Repair:        cur_repair = &c; addPost(c.c_type, c.tag, c.para, ctx); break;
                case ModuleType::T_evaluator:     eval_since_gen = true; addPost(c.c_type, c.tag, c.para, ctx); break;

                case ModuleType::T_offspringgenerator:
                {
                    std::string g = "generator '" + c.tag + "'";
                    // N2:前置组件须已 SET(吸收 REPAIR-REQUIRED)
                    if (!cur_topo)  { r.errors.push_back(g + ": no topology set before it"); r.valid = false; }
                    if (!cur_strat) { r.errors.push_back(g + ": no strategy set before it"); r.valid = false; }
                    if (!cur_repair){ r.errors.push_back(g + ": no repair set before it"); r.valid = false; }
                    // N1:generator 处统一校 所用 topology/strategy/repair + generator 自身 的 preAssert
                    if (cur_topo)  checkComponent(ModuleType::T_learntopology, cur_topo->tag, cur_topo->para, ctx, "topology '" + cur_topo->tag + "'", r);
                    if (cur_strat) checkComponent(ModuleType::T_learnstrategy, cur_strat->tag, cur_strat->para, ctx, "strategy '" + cur_strat->tag + "'", r);
                    if (cur_repair)checkComponent(ModuleType::T_Repair, cur_repair->tag, cur_repair->para, ctx, "repair '" + cur_repair->tag + "'", r);
                    checkComponent(ModuleType::T_offspringgenerator, c.tag, c.para, ctx, g, r);
                    addPost(c.c_type, c.tag, c.para, ctx);
                    // N3 状态:本次 generator 后,gen 已现、eval 归零
                    n_gen++; gen_since_sel = true; eval_since_gen = false;
                    break;
                }
                case ModuleType::T_selector:
                {
                    std::string s = "selector '" + c.tag + "'";
                    // N3:选择前须有(上次选择后的)generator,且该 generator 后须有 evaluator
                    if (!gen_since_sel)
                    { r.errors.push_back(s + ": no generator since last selector (nothing to select)"); r.valid = false; }
                    else if (!eval_since_gen)
                    { r.errors.push_back(s + ": no evaluator after last generator (unevaluated offspring)"); r.valid = false; }
                    checkComponent(ModuleType::T_selector, c.tag, c.para, ctx, s, r);
                    addPost(c.c_type, c.tag, c.para, ctx);
                    // 重置
                    n_sel++; gen_since_sel = false; eval_since_gen = false;
                    break;
                }

                default: break;
                }
            }

            // N4:结构完整性(≥1 generator 防死循环退不出;≥1 selector 子代无从并入亲代)
            if (n_gen < 1) { r.errors.push_back("workflow has no generator (offspring never produced -> cannot terminate by FES)"); r.valid = false; }
            if (n_sel < 1) { r.errors.push_back("workflow has no selector (offspring never merged into parent)"); r.valid = false; }

            cache[wf.tag] = r;
            return r;
        }
    };
}
